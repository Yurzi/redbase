#include <cstring>

#include <iostream>

#include "pf_error.h"

static const char *PFWarnMsg[] = {
  "page pinned in buffer",
  "page is not in the buffer",
  "invalid page number",
  "file open",
  "invalid file descriptor (file closed)",
  "page already free",
  "page already unpinned",
  "end of file",
  "attempting to resize the buffer too small",
  "invalid filename"
};

static const char *PFErrorMsg[] = {
  "no memory",
  "no buffer space",
  "incomplete read of page from file",
  "incomplete write of page to file",
  "incomplete read of header from file",
  "incomplete write of header from file",
  "new page to be allocated already in buffer",
  "hash table entry not found",
  "page already in hash table",
  "invalid file name"
};

void PFPrintError(RC rc) {
  if (rc >= START_PF_WARN && rc <= PF_LASTWARN) {
    // Check the return code is within proper limits
    // Print warning
    std::cerr << "PF warning: " << PFWarnMsg[rc - START_PF_WARN] << "\n";
  } else if (-rc >= -START_PF_ERR && -rc < -PF_LASTERROR) {
    // Error codes are negative, so invert everything
    // Print error
    std::cerr << "PF error: " << PFErrorMsg[-rc + START_PF_ERR] << "\n";
  } else if (rc == PF_UNIX) {
    std::cerr << std::strerror(errno) << "\n";
  } else if (rc == 0) {
    std::cerr << "PFPrintError called with return code of 0\n";
  } else {
    std::cerr << "PF error: " << rc << " is out of bounds\n";
  }
}
