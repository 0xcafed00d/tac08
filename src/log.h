#ifndef LOG_H
#define LOG_H

#include <functional>
#include <iostream>
#include <sstream>

enum class level {
	perf = 1,
	trace = 2,
	info = 4,
	err = 8,
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
		};

	   private:
		Logger& m_logger;
	};
	friend LogProxy;

	template <typename T>
	LogProxy operator<<(const T& v) {
		if (m_enabled) {
			m_str << v;
		}
		return LogProxy(*this);
	};

	bool enabled() const {
		return m_enabled;
	}

	void enable(bool enable) {
		m_enabled = enable;
	}

	void setOutputFunction(std::function<void(const char*)> outFunc) {
		m_outputFunc = outFunc;
	}

   private:
	void flush() {
		if (m_enabled) {
			if (m_outputFunc) {
				m_outputFunc(m_str.str().c_str());
			} else {
				std::cerr << m_str.str() << std::endl;
			}
			m_str.str("");
			m_str.clear();
		}
	}

	std::stringstream m_str;
	int m_proxycount = 0;
	bool m_enabled = true;
	std::function<void(const char*)> m_outputFunc;
};

extern Logger logr;

#define TraceFunction() logr << "TRACEFUNC: " << __PRETTY_FUNCTION__

#endif /* LOG_H */
