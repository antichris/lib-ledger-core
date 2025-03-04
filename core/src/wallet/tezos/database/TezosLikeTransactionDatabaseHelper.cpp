/*
 *
 * TezosLikeTransactionDatabaseHelper
 *
 * Created by El Khalil Bellakrid on 27/04/2019.
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Ledger
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */


#include "TezosLikeTransactionDatabaseHelper.h"
#include <api/enum_from_string.hpp>
#include <database/soci-option.h>
#include <database/soci-date.h>
#include <database/soci-number.h>
#include <crypto/SHA256.hpp>
#include <wallet/common/database/BlockDatabaseHelper.h>
#include <wallet/common/database/OperationDatabaseHelper.h>
#include <utils/Option.hpp>
#include <api/TezosOperationTag.hpp>
using namespace soci;

namespace ledger {
    namespace core {

        bool TezosLikeTransactionDatabaseHelper::getTransactionByHash(soci::session &sql,
                                                                      const std::string &hash,
                                                                      const std::string &operationUid,
                                                                      TezosLikeBlockchainExplorerTransaction &tx) {

            rowset<row> rows = (sql.prepare << "SELECT tx.hash, tx.value, tx.time, "
                    " tx.sender, tx.receiver, tx.fees, tx.gas_limit, tx.storage_limit, tx.confirmations, tx.type, tx.public_key, tx.originated_account, tx.status, "
                    "block.height, block.hash, block.time, block.currency_name "
                    "FROM tezos_transactions AS tx "
                    "LEFT JOIN blocks AS block ON tx.block_uid = block.uid "
                    "LEFT JOIN tezos_operations AS xtz_ops ON xtz_ops.transaction_uid = tx.transaction_uid "
                    "WHERE tx.hash = :hash AND xtz_ops.uid = :uid", use(hash), use(operationUid));

            for (auto &row : rows) {
                inflateTransaction(sql, row, tx);
                return true;
            }

            return false;
        }

        bool TezosLikeTransactionDatabaseHelper::inflateTransaction(soci::session &sql,
                                                                    const soci::row &row,
                                                                    TezosLikeBlockchainExplorerTransaction &tx) {
            tx.hash = row.get<std::string>(0);
            tx.value = BigInt::fromHex(row.get<std::string>(1));
            tx.receivedAt = row.get<std::chrono::system_clock::time_point>(2);
            tx.sender = row.get<std::string>(3);
            tx.receiver = row.get<std::string>(4);
            tx.fees = BigInt::fromHex(row.get<std::string>(5));
            tx.gas_limit = BigInt::fromHex(row.get<std::string>(6));
            tx.storage_limit = BigInt::fromHex(row.get<std::string>(7));
            tx.confirmations = get_number<uint64_t>(row, 8);
            tx.type = api::from_string<api::TezosOperationTag>(row.get<std::string>(9));
            auto pubKey = row.get<std::string>(10);
            if (!pubKey.empty()) {
                tx.publicKey = pubKey;
            }
            auto values = strings::split(row.get<std::string>(11), ":");
            if (values.size() == 3) {
                tx.originatedAccount = TezosLikeBlockchainExplorerOriginatedAccount(values[0], static_cast<bool>(std::stoi(values[1])), static_cast<bool>(std::stoi(values[2])));
            }

            tx.status = get_number<uint64_t>(row, 12);
            if (row.get_indicator(13) != i_null) {
                TezosLikeBlockchainExplorer::Block block;
                block.height = get_number<uint64_t>(row, 13);
                block.hash = row.get<std::string>(14);
                block.time = row.get<std::chrono::system_clock::time_point>(15);
                block.currencyName = row.get<std::string>(16);
                tx.block = block;
            }

            return true;
        }

        bool TezosLikeTransactionDatabaseHelper::transactionExists(soci::session &sql,
                                                                   const std::string &tezosTxUid) {
            int32_t count = 0;
            sql << "SELECT COUNT(*) FROM tezos_transactions WHERE transaction_uid = :tezosTxUid", use(tezosTxUid), into(
                    count);
            return count == 1;
        }

        std::string TezosLikeTransactionDatabaseHelper::createTezosTransactionUid(const std::string &accountUid,
                                                                                  const std::string &txHash,
                                                                                  api::TezosOperationTag type) {
            auto result = SHA256::stringToHexHash(fmt::format("uid:{}+{}+{}", accountUid, txHash, api::to_string(type)));
            return result;
        }

        std::string TezosLikeTransactionDatabaseHelper::putTransaction(soci::session &sql,
                                                                       const std::string &accountUid,
                                                                       const TezosLikeBlockchainExplorerTransaction &tx) {
            auto blockUid = tx.block.map<std::string>([](const TezosLikeBlockchainExplorer::Block &block) {
                return block.getUid();
            });

            auto tezosTxUid = createTezosTransactionUid(accountUid, tx.hash, tx.type);

            if (transactionExists(sql, tezosTxUid)) {
                // UPDATE (we only update block information)
                if (tx.block.nonEmpty()) {
                    auto type = api::to_string(tx.type);
                    sql << "UPDATE tezos_transactions SET block_uid = :uid, status = :code WHERE hash = :tx_hash AND type = :type",
                            use(blockUid), use(tx.status), use(tx.hash), use(type);
                }
                return tezosTxUid;
            } else {
                // Insert
                if (tx.block.nonEmpty()) {
                    BlockDatabaseHelper::putBlock(sql, tx.block.getValue());
                }
                auto hexValue = tx.value.toHexString();
                auto hexFees = tx.fees.toHexString();
                auto hexGasLimit = tx.gas_limit.toHexString();
                auto hexStorageLimit = tx.storage_limit.toHexString();
                auto type = api::to_string(tx.type);
                auto pubKey = tx.publicKey.getValueOr("");

                std::string sOrigAccount;
                if (tx.originatedAccount.hasValue()) {
                    std::stringstream origAccount;
                    std::vector<std::string> vOrigAccount{tx.originatedAccount.getValue().address, std::to_string(tx.originatedAccount.getValue().spendable), std::to_string(tx.originatedAccount.getValue().delegatable)};
                    strings::join(vOrigAccount, origAccount, ":");
                    sOrigAccount = origAccount.str();
                }
                sql << "INSERT INTO tezos_transactions VALUES(:tx_uid, :hash, :value, :block_uid, :time, :sender, :receiver, :fees, :gas_limit, :storage_limit, :confirmations, :type, :public_key, :originated_account, :status)",
                        use(tezosTxUid),
                        use(tx.hash),
                        use(hexValue),
                        use(blockUid),
                        use(tx.receivedAt),
                        use(tx.sender),
                        use(tx.receiver),
                        use(hexFees),
                        use(hexGasLimit),
                        use(hexStorageLimit),
                        use(tx.confirmations),
                        use(type),
                        use(pubKey),
                        use(sOrigAccount),
                        use(tx.status);

                return tezosTxUid;
            }
        }

        void TezosLikeTransactionDatabaseHelper::eraseDataSince(
                    soci::session &sql,
                    const std::string &accountUid,
                    const std::chrono::system_clock::time_point & date) {
                        
            OperationDatabaseHelper::eraseDataSince(sql, accountUid, date, 
                "tezos_operations", "tezos_transactions");
        }
    }
}