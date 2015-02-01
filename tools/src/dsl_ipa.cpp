#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include "dsl_ipa.h"

static UniToStrPair ipa_to_unicode_make_pair(gunichar from,
                                             gunichar to)
{
        gchar buf[7];

        UniToStrPair res;
        res.first = from;
        buf[g_unichar_to_utf8(to, buf)] = '\0';
        res.second =buf;

        return res;
}

static UniToStrPair ipa_to_unicode_make_pair(gunichar from,
                                             gunichar to1,
                                             gunichar to2)
{
        gchar buf[7];
        
        UniToStrPair res;
        res.first = from;
        buf[g_unichar_to_utf8(to1, buf)] = '\0';
        res.second = buf;
        buf[g_unichar_to_utf8(to2, buf)] = '\0';
        res.second += buf;

        return res;
}

std::pair<UniToStrPair *, UniToStrPair *> ipa_to_unicode_tbl()
{
        static UniToStrPair ipa_to_unicode_tbl[] = {
		ipa_to_unicode_make_pair('\'',  0x02c8),
		ipa_to_unicode_make_pair(0x00a0,  0x02A7),
		ipa_to_unicode_make_pair(0x00a4,  0x0062),
		ipa_to_unicode_make_pair(0x00a6,  0x0077),
		ipa_to_unicode_make_pair(0x00a7,  0x0066),
		ipa_to_unicode_make_pair(0x00a9,  0x0073),
		ipa_to_unicode_make_pair(0x00ab,  0x0074),
		ipa_to_unicode_make_pair(0x00ac,  0x0064),
		ipa_to_unicode_make_pair(0x00ad,  0x006e),
		ipa_to_unicode_make_pair(0x00ae,  0x006c),
		ipa_to_unicode_make_pair(0x00b0,  0x006b),
		ipa_to_unicode_make_pair(0x00b1,  0x0261),
		ipa_to_unicode_make_pair(0x00b5,  0x0061),
		ipa_to_unicode_make_pair(0x0402,  0x0069, ':'),
		ipa_to_unicode_make_pair(0x0403,  0x0251, ':' ),
		ipa_to_unicode_make_pair(0x0404,  0x007a),
		ipa_to_unicode_make_pair(0x0406,  0x0068),
		ipa_to_unicode_make_pair(0x0407,  0x0072),
		ipa_to_unicode_make_pair(0x0408,  0x0070),
		ipa_to_unicode_make_pair(0x0409,  0x0292),
		ipa_to_unicode_make_pair(0x040a,  0x014b),
		ipa_to_unicode_make_pair(0x040b,  0x03b8),
		ipa_to_unicode_make_pair(0x040c,  0x0075),
		ipa_to_unicode_make_pair(0x040e,  0x026a),
		ipa_to_unicode_make_pair(0x040f,  0x0283),
		ipa_to_unicode_make_pair(0x0428,  0x0061),
		ipa_to_unicode_make_pair(0x0452,  0x0076),
		ipa_to_unicode_make_pair(0x0453,  0x0075, ':'),
		ipa_to_unicode_make_pair(0x0456,  0x006a),
		ipa_to_unicode_make_pair(0x045e,  0x0065),
		ipa_to_unicode_make_pair(0x0490,  0x006d),
		ipa_to_unicode_make_pair(0x0491,  0x025b),
		ipa_to_unicode_make_pair(0x201a,  0x0254),
		ipa_to_unicode_make_pair(0x201e,  0x0259),
		ipa_to_unicode_make_pair(0x2020,  0x0259),
		ipa_to_unicode_make_pair(0x2021,  0x00e6),
		ipa_to_unicode_make_pair(0x2026,  0x028c),
		ipa_to_unicode_make_pair(0x2030,  0x00f0),
		ipa_to_unicode_make_pair(0x2039,  0x0064, 0x0292),
		ipa_to_unicode_make_pair(0x20ac,  0x0254),
       };

        return std::make_pair(ipa_to_unicode_tbl, ipa_to_unicode_tbl +
                              sizeof(ipa_to_unicode_tbl) / sizeof(UniToStrPair));
}
