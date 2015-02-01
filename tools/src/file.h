#ifndef _FILE_HPP_
#define _FILE_HPP_

#include <cstdio>
#include <string>

/**
 * Wrapper around FILE, because of std::fstream can not
 * exactly explain what error happened, but there is guarantee
 * that errno contains error for FILE functions.
 */
class File {
public:
	File(FILE *stream = NULL, bool close_on_exit = true) : 
		stream_(stream), close_on_exit_(close_on_exit) {}
	void close() {
		if (stream_ && close_on_exit_) {
			fclose(stream_);
			stream_ = NULL;
		}
	}
	File& reset(FILE *stream) {
		close();
		stream_ = stream;
		return *this;
	}
	~File() { 
		close();
	}
	File& operator<<(const std::string&);
	File& operator<<(const char *str);
	File& operator<<(double d) {
		fprintf(stream_, "%lf", d);
		return *this;
	}
	File& operator<<(char ch) {
		fputc(ch, stream_);
		return *this;
	}
	File& operator<<(unsigned int i) {
		fprintf(stream_, "%u", i);
		return *this;
	}
	void flush();
	File& printf(const char *fmt, ...);
	bool operator!() const { return !stream_ || feof(stream_) || ferror(stream_); }
	void not_use_buffer() { setbuf(stream_, NULL); }
private:
	struct Tester {
		Tester() {}
        private:
		void operator delete(void*);
        };
public:
	// enable 'if (sp)'
        operator Tester*() const
        {
            if (!*this) return 0;
            static Tester t;
            return &t;
        }
	static File& getline(File& in, std::string& line);
	File& write(const char *buf, size_t len);
	File& read(char *buf, size_t len) {
		size_t fread_size;
		fread_size = fread(buf, 1, len, stream_);
		if (fread_size != (size_t)len) {
			 g_print("fread error!\n");
		}
		return *this;
	}
	bool seek(long off) {
		return fseek(stream_, off, SEEK_SET) == 0;
	}
	long tell() { return ftell(stream_); }
private:
	FILE *stream_;
	bool close_on_exit_;
};

class Strip {
public:
	explicit Strip(const std::string& str) : str_(str) {}
	friend File& operator<<(File& out, const Strip& st);
private:
	const std::string& str_;
};

extern File StdIn;
extern File StdOut;

#endif//!_FILE_HPP_
