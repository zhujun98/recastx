#ifndef TOMCAT_LOGGER_HPP
#define TOMCAT_LOGGER_HPP

#include <tuple>

#include <spdlog/spdlog.h>

#include <spdlog/details/null_mutex.h>
#include <spdlog/sinks/base_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/details/synchronous_factory.h>

namespace recastx::gui {

namespace details {

template<typename Mutex>
class CallbackSink final : public spdlog::sinks::base_sink<Mutex> {
public:

    using log_callback = std::function<void(const spdlog::details::log_msg &msg)>;

    explicit CallbackSink(const log_callback& callback)
            : callback_{callback} {}

protected:
    void sink_it_(const spdlog::details::log_msg &msg) override {
        callback_(msg);
    }

    void flush_() override {};

private:

    log_callback callback_;
};

using CallbackSinkMt = CallbackSink<std::mutex>;

} // details

class GuiLogger {

public:

    using BufferType = std::vector<std::tuple<spdlog::level::level_enum, std::string>>;

private:

    inline static std::unique_ptr<spdlog::logger> logger_;

public:

    GuiLogger() = default;
    ~GuiLogger() = default;

    inline static void init(BufferType& buf) {
        auto callback_sink = std::make_shared<details::CallbackSinkMt>(
                [&buf](const spdlog::details::log_msg &msg) {
                    buf.emplace_back(msg.level, std::string(msg.payload.begin(), msg.payload.end()));
                });

        auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();

        logger_ = std::make_unique<spdlog::logger>(
                "GUI logger", spdlog::sinks_init_list{callback_sink, console_sink});
        logger_->set_pattern("[%Y-%m-%d %T.%e] [%^%l%$] %v");
#ifndef NDEBUG
        logger_->set_level(spdlog::level::debug);
#endif
    }

    static spdlog::logger* ptr() { return logger_.get(); }

};

namespace log {

template<typename... Args>
inline void debug(Args &&... args) {
    GuiLogger::ptr()->debug(std::forward<Args>(args)...);
}

template<typename... Args>
inline void info(Args &&... args) {
    GuiLogger::ptr()->info(std::forward<Args>(args)...);
}

template<typename... Args>
inline void warn(Args &&... args) {
    GuiLogger::ptr()->warn(std::forward<Args>(args)...);
}

template<typename... Args>
inline void error(Args &&... args) {
    GuiLogger::ptr()->error(std::forward<Args>(args)...);
}

} // log

} // namespace recastx::gui

#endif //TOMCAT_LOGGER_HPP
