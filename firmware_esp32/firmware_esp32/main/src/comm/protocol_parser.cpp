#include "protocol_parser.h"

bool parsePacket(const String& raw, ParsedMsg& out) {
  String s = raw;
  s.trim();
  if (s.length() == 0) return false;

  // Preferred: Mega protocol -> KEY:VALUE
  int sep = s.indexOf(':');
  if (sep > 0) {
    out.key = s.substring(0, sep);
    out.value = s.substring(sep + 1);
    out.key.trim();
    out.value.trim();
    return out.key.length() > 0;
  }

  // Backward compatible: <KEY=VALUE>
  if (s.startsWith("<") && s.endsWith(">")) {
    String body = s.substring(1, s.length() - 1);
    int sep2 = body.indexOf('=');
    if (sep2 < 0) return false;
    out.key = body.substring(0, sep2);
    out.value = body.substring(sep2 + 1);
    out.key.trim();
    out.value.trim();
    return out.key.length() > 0;
  }

  return false;
}
