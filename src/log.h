#ifndef LOG_H
#define LOG_H

#include <functional>
#include <iostream>
#include <sstream>

enum class LogLevel : uint32_t {
	perf = 1,
	trace = 2,
	info = 4,
	err = 8,
	apitrace = 16,
};

class Logger {
   public:
	class LogProxy {
	   public:
		LogProxy(Logger& logger) : m_logger(logger) {
			m_logger.m_proxycount++;
		}

		~LogProxy() {
			m_logger.m_proxycount--;

			if (m_logger.m_proxycount == 0) {
				m_logger.flush();
			}
		}
		LogProxy(const LogProxy& that) : m_logger(that.m_logger) {
			m_logger.m_proxycount++;
		}

		template <typename T>
		LogProxy operator<<(const T& v) {
			m_logger << v;
			m_logger << " ";
			return LogProxy(m_logger);
		}

	   private:
		Logger& m_logger;
	};
	friend LogProxy;

	template <typename T>
	LogProxy operator<<(const T& v) {
		if (ok()) {
			m_str << v;
		}
		return LogProxy(*this);
	};

	LogProxy operator<<(LogLevel l) {
		m_level = l;
		return LogProxy(*this);
	}

	bool enabled() const {
		return m_enabled;
	}

	void enable(bool enable) {
		m_enabled = enable;
	}

	void setOutputFunction(std::function<void(LogLevel, const char*)> outFunc) {
		m_outputFunc = outFunc;
	}

	void setOutputFilter(LogLevel l, bool on) {
		uint32_t n = (uint32_t)l;
		if (on) {
			m_logfilter |= n;
		} else {
			m_logfilter &= ~n;
		}
	}

   private:
	void flush() {
		if (ok()) {
			if (m_outputFunc) {
				m_outputFunc(m_level, m_str.str().c_str());
			} else {
				std::cerr << m_str.str() << std::endl;
			}
			m_str.str("");
			m_str.clear();
		}
		m_level = LogLevel::info;
	}

	bool ok() {
		return m_enabled && ((uint32_t)m_level & (uint32_t)m_logfilter);
	}

	std::stringstream m_str;
	int m_proxycount = 0;
	bool m_enabled = true;
	LogLevel m_level = LogLevel::info;
	uint32_t m_logfilter = 0xff;
	std::function<void(LogLevel, const char*)> m_outputFunc;
};

extern Logger logr;

class logr_trace_func__ {
	const char* fname;

   public:
	logr_trace_func__(const char* fname) : fname(fname) {
		logr << LogLevel::trace << " enter >>: " << fname;
	}
	~logr_trace_func__() {
		logr << LogLevel::trace << " leave <<: " << fname;
	}
};

#ifdef _MSC_VER
#define TraceFunction() logr_trace_func__ trace_func_log__(__FUNCSIG__)
#else
#define TraceFunction() logr_trace_func__ trace_func_log__(__PRETTY_FUNCTION__)
#endif

#endif /* LOG_H */
