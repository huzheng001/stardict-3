#ifndef _DSL_IPA_HPP_
#define _DSL_IPA_HPP_

#include <glib.h>
#include <utility>
#include <string>

typedef std::pair<gunichar, std::string> UniToStrPair;

extern std::pair<UniToStrPair *, UniToStrPair *> ipa_to_unicode_tbl();

#endif//!_DSL_IPA_HPP_
