// Scintilla source code edit control
/** @file PropSetSimple.cxx
 ** A basic string to string map.
 **/
// Copyright 1998-2010 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

// Maintain a dictionary of properties

#include <cstdlib>
#include <cstring>

#include <string>
#include <string_view>
#include <map>

#include "PropSetSimple.h"

using namespace Lexilla;

namespace {

typedef std::map<std::string, std::string> mapss;

mapss *PropsFromPointer(void *impl) noexcept {
	return static_cast<mapss *>(impl);
}

}

PropSetSimple::PropSetSimple() {
	mapss *props = new mapss;
	impl = static_cast<void *>(props);
}

PropSetSimple::~PropSetSimple() {
	mapss *props = PropsFromPointer(impl);
	delete props;
	impl = nullptr;
}

void PropSetSimple::Set(std::string_view key, std::string_view val) {
	mapss *props = PropsFromPointer(impl);
	if (!props)
		return;
	(*props)[std::string(key)] = std::string(val);
}

const char *PropSetSimple::Get(std::string_view key) const {
	mapss *props = PropsFromPointer(impl);
	if (props) {
		mapss::const_iterator keyPos = props->find(std::string(key));
		if (keyPos != props->end()) {
			return keyPos->second.c_str();
		}
	}
	return "";
}

int PropSetSimple::GetInt(std::string_view key, int defaultValue) const {
	const char *val = Get(key);
	if (*val) {
		return atoi(val);
	}
	return defaultValue;
}
