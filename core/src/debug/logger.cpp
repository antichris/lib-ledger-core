/*
 *
 * logger
 * ledger-core
 *
 * Created by Pierre Pollastri on 24/11/2016.
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2016 Ledger
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

#include "logger.hpp"
#include <spdlog/spdlog.h>
#include <spdlog/sinks/null_sink.h>
#include "LogPrinterSink.hpp"
#include "RotatingEncryptableSink.hpp"
#include "api/PathResolver.hpp"
#include <memory>
#include "api/ExecutionContext.hpp"

namespace ledger {
    namespace core {
        std::shared_ptr<spdlog::logger> logger::create(
            const std::string &name,
            const std::shared_ptr<api::ExecutionContext> &context,
            const std::shared_ptr<api::PathResolver> &resolver,
            const std::shared_ptr<api::LogPrinter> &printer,
            size_t maxSize,
            bool enabled
        ) {
            if (enabled) {
                std::vector<spdlog::sink_ptr> sinks;
                sinks.push_back(std::make_shared<LogPrinterSink>(printer));
                sinks.push_back(std::make_shared<RotatingEncryptableSink>(context, resolver, name, maxSize, 3));
                auto logger = std::make_shared<spdlog::logger>(name, begin(sinks), end(sinks));
                spdlog::drop(name);

                logger->set_level(spdlog::level::trace);
                logger->flush_on(spdlog::level::trace);
                logger->set_pattern("%Y-%m-%dT%XZ%z %L: %v");
                return logger;
            } else {
                auto logger = spdlog::create<spdlog::sinks::null_sink_st>(name);
                spdlog::drop(name);

                logger->set_level(spdlog::level::off);
                return logger;
            }
        }

        std::shared_ptr<spdlog::logger>
        logger::trace(const std::string& purpose, const std::string &tracePrefix, const std::shared_ptr<spdlog::logger> &logger) {
            auto name = fmt::format("{}_{}", logger->name(), purpose);
            std::vector<spdlog::sink_ptr> sinks;
            for (const auto& sink : logger->sinks()) {
                auto printer = std::dynamic_pointer_cast<LogPrinterSink>(sink);
                if (printer) {
                    auto apiPrinter = printer->getPrinter().lock();
                    if (apiPrinter) {
                        sinks.push_back(std::make_shared<LogPrinterSink>(apiPrinter));
                    }
                }

            }
            auto traceLogger = std::make_shared<spdlog::logger>(name, begin(sinks), end(sinks));
            spdlog::drop(name);
            traceLogger->set_level(spdlog::level::trace);
            traceLogger->flush_on(spdlog::level::trace);
            traceLogger->set_pattern(fmt::format("%Y-%m-%dT%XZ%z %L: [{}] %v", tracePrefix));
            return traceLogger;
        }
    }
}
