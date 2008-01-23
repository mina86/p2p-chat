/** \file
 * An "User interface" module playing sounds.
 * $Id: sounds.cpp,v 1.1 2008/01/23 03:02:45 mina86 Exp $
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "sounds.hpp"
#include "config.hpp"


namespace ppc {


static void playSound(const std::string &dir, const std::string &file);


unsigned SoundsUI::seq = 0;

int SoundsUI::setFDSets(fd_set *rd, fd_set *wr, fd_set *ex) {
	(void)rd; (void)wr; (void)ex;
	return 0;
}

int SoundsUI::doFDs(int nfds, const fd_set *rd, const fd_set *wr,
                    const fd_set *ex) {
	(void)nfds; (void)rd; (void)wr; (void)ex;
	return 0;
}

void SoundsUI::recievedSignal(const Signal &sig) {
	if (sig.getType() == "/net/status/changed") {
		const sig::UserData &data = *sig.getData<sig::UserData>();
		if (!(data.flags & (sig::UserData::CONNECTED | sig::UserData::DISCONNECTED)) &&
		    data.user.id.address.ip) {
			playSound(getConfig().getString("sounds/directory", "sounds"),
			          getConfig().getString("sounds/files/status-changed",
			                                "status-changed.wav"));
		}

	} else if (sig.getType() == "/net/msg/got") {
		playSound(getConfig().getString("sounds/directory", "sounds"),
		          getConfig().getString("sounds/files/got-message",
		                                "got-message.wav"));

	} else if (sig.getType() == "/core/module/quit") {
		sendSignal("/core/module/exits", Core::coreName);

	}
}


void playSound(const std::string &dir, const std::string &file) {
	struct stat buf;
	std::string path;
	int fd;

	fprintf(stderr, "%s %s\n", dir.c_str(), file.c_str());

	if (file.empty()) {
		return;
	}

	if (file[0]=='/') {
		path = file;
	} else {
		path = dir;
		path += '/';
		path.append(file);
	}

	fprintf(stderr, "%s\n", path.c_str());

	if (stat(path.c_str(), &buf)<0 || fork()) {
		return;
	}

	fd = sysconf(_SC_OPEN_MAX);
	for (int i = 2; ++i<fd; close(i));

	fd = open("/dev/null", O_RDWR);
	if (fd<0 || dup2(fd, 0)<0 || dup2(fd, 1)<0 || dup2(fd, 2)<0) {
		_exit(1);
	}

	setsid();
	switch (fork()) {
	case -1: _exit(1);
	case  0: break;
	default: _exit(0);
	}

	execlp("aplay", "aplay", path.c_str(), (const char*)0);
}



}
