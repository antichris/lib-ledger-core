/*
 *
 * RIppleLikeTransactionDatabaseHelper
 *
 * Created by El Khalil Bellakrid on 06/01/2019.
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


#ifndef LEDGER_CORE_RIPPLELIKETRANSACTIONDATABASEHELPER_H
#define LEDGER_CORE_RIPPLELIKETRANSACTIONDATABASEHELPER_H


#include <string>
#include <soci.h>
#include <wallet/ripple/explorers/RippleLikeBlockchainExplorer.h>

namespace ledger {
    namespace core {
        class RippleLikeTransactionDatabaseHelper {
        public:
            static bool getTransactionByHash(soci::session &sql,
                                             const std::string &hash,
                                             RippleLikeBlockchainExplorerTransaction &tx);

            static bool inflateTransaction(soci::session &sql,
                                           const soci::row &row,
                                           RippleLikeBlockchainExplorerTransaction &tx);

            static bool transactionExists(soci::session &sql,
                                          const std::string &rippleTxUid);

            static std::string createRippleTransactionUid(const std::string &accountUid,
                                                          const std::string &txHash);

            static std::string putTransaction(soci::session &sql,
                                              const std::string &accountUid,
                                              const RippleLikeBlockchainExplorerTransaction &tx);

            static void eraseDataSince( soci::session &sql,
                                const std::string &accountUid,
                                const std::chrono::system_clock::time_point & date);
        };
    }
}

#endif //LEDGER_CORE_RIPPLELIKETRANSACTIONDATABASEHELPER_H
