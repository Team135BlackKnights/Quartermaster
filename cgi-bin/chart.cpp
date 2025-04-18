#include "chart.h"
#include<iostream>
#include<unistd.h>
#include<array>
#include<cassert>
#include<sstream>
#include<sys/wait.h>
#include "home.h"
#include "progress.h"
#include "subsystem.h"
using namespace std;

void inner(std::ostream& o,Chart const&,DB db){
	string user = current_user(db);
	if (user == "no_user") {
		cout << "Location: /cgi-bin/parts.cgi?p=Login\n\n";
	}
	return make_page(
		o,
		"Progress chart",
		"<img src=\"?p=Chart_image\">"
	);
}

std::array<int,2> pipe(){
	std::array<int,2> fds;
	{
		auto r=pipe(&fds[0]);
		assert(r==0);
	}
	return fds;
}

void dup2_or_die(int oldfd,int newfd){
	auto r=dup2(oldfd,newfd);
	assert(r==newfd);
}

void close_or_die(int fd){
	auto r=close(fd);
	assert(r==0);
}

//struct ostreamfd:std::ostream,std::streambuf{
//struct ostreamfd:std::basic_streambuf<char>{
struct ostreamfd : std::streambuf {
    int fd;

public:
    explicit ostreamfd(int fd1) : fd(fd1) {
        assert(fd != -1);
    }

    ~ostreamfd() {
        close_or_die(fd);
    }

    int get() const {
        return fd;
    }

    // Implement overflow() to write a single character
    int overflow(int c) override {
        if (c != EOF) {
            char ch = c;
            ssize_t written = write(fd, &ch, 1);
            if (written != 1) {
                return EOF;
            }
        }
        return c;
    }

    // Implement sync() to flush the stream (if needed)
    int sync() override {
        // In this simple implementation, there's no internal buffering,
        // so we can simply return 0 to indicate success.
        return 0;
    }

    // Write directly using write() for block output.
    std::streamsize xsputn(const char* s, std::streamsize len) override {
        return write(fd, s, len);
    }
};


/*template<typename T>
ostreamfd& operator<<(ostreamfd& a,T const& t){
	stringstream ss;
	ss<<t;
	int r=write(a.get(),ss.str().c_str(),ss.str().size());
	assert(unsigned(r)==ss.str().size());
	return a;
}*/

void run(DB db){
	cout<<"Content-type: image/png\n";
	cout<<"Expires: 0\n\n";
	cout.flush();
	auto fds=pipe();
	pid_t pid=fork();
	assert(pid!=-1);
	
	if(pid==0){
		//child process
		close_or_die(0);
		dup2_or_die(fds[0],0);
		close_or_die(fds[1]);

		int r=execlp("./area3.py","./area3.py","--in",NULL);
		assert(r==-1);

		assert(0);
	}
	close_or_die(fds[0]);
	{
		ostreamfd to_sub1(fds[1]);
		std::ostream to_sub(&to_sub1);
		/*for(auto _:range(10)){
			(void)_;
			write(fds[1],"this\n",5);
		}*/
		to_sub<<"Number of parts in each state by date\n";
		to_sub<<"Axis label1\n";
		to_sub<<"Axis (y)?\n";
		in_state_by_date(to_sub,db);
		//to_sub<<"hello there";
	}
	//sleep(5);

	int wstatus;
	pid_t r=waitpid(pid,&wstatus,0);
	assert(r==pid);
	if(wstatus){
		cerr<<"Subprocess status:"<<wstatus<<"\n";
	}
	assert(wstatus==0);
}

void inner(std::ostream& o,Chart_image const&,DB db){
	string user = current_user(db);
	if (user == "no_user") {
		cout << "Location: /cgi-bin/parts.cgi?p=Login\n\n";
	}
	run(db);
}

