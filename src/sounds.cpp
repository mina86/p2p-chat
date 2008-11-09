/** \file
 * An "User interface" module playing sounds.
 * Copyright 2008 by Michal Nazarewicz (mina86/AT/mina86.com)
 *
 * This program is free software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
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
			playSound(getConfig().getString("config/sounds/directory", "sounds"),
			          getConfig().getString("config/sounds/files/status-changed",
			                                "status-changed.wav"));
		}

	} else if (sig.getType() == "/net/msg/got") {
		playSound(getConfig().getString("config/sounds/directory", "sounds"),
		          getConfig().getString("config/sounds/files/got-message",
		                                "got-message.wav"));

	} else if (sig.getType() == "/core/module/quit") {
		sendSignal("/core/module/exits", Core::coreName);

	}
}


void playSound(const std::string &dir, const std::string &file) {
	struct stat buf;
	std::string path;
	int fd;

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
