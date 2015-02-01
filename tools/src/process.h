#ifndef __PROCESS_HPP__
#define __PROCESS_HPP__

#include <glib.h>
#include <string>

#include "file.h"

class Process {
public:
	enum {
		OPEN_PIPE_FOR_READ = 1 << 0,
		OPEN_PIPE_FOR_WRITE = 1 << 1,
		INHERIT_STDIN = 1 << 2
	};
	bool run_async(const std::string& cmd, int flags);
	bool wait(int &res);
	File& input() { return input_; }
	File& output() { return output_; }
	enum ResultValue {
	  rvEXIT_SUCCESS = EXIT_SUCCESS,
	  rvEXIT_FAILURE = EXIT_FAILURE,
	  rvEXEC_FAILED = (EXIT_FAILURE + EXIT_SUCCESS) * 2
	};
	static ResultValue run_cmd_line_sync(const std::string& cmd,
					     std::string& output, GError **err);
private:
	File input_, output_;
	GPid pid_;
};

#endif//!__PROCESS_HPP__
