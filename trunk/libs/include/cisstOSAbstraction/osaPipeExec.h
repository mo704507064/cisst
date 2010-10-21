/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-    */
/* ex: set filetype=cpp softtabstop=4 shiftwidth=4 tabstop=4 cindent expandtab: */

/*
  $Id $

  Author(s): Martin Kelly
  Created on: 2010-09-23

  (C) Copyright 2010 Johns Hopkins University (JHU), All Rights
  Reserved.

--- begin cisst license - do not edit ---

This software is provided "as is" under an open source license, with
no warranty.  The complete license can be found in license.txt and
http://www.cisst.org/cisst/license.txt.

--- end cisst license ---

*/

#ifndef _osaPipeExec_h
#define _osaPipeExec_h

#ifndef READ_HANDLE
#define READ_HANDLE 0
#endif
#ifndef WRITE_HANDLE
#define WRITE_HANDLE 1
#endif

class osaPipeExec {
	/*! Internals that are OS-dependent */
	enum {INTERNALS_SIZE = 32};
	char Internals[INTERNALS_SIZE];

	/*! Return the size of the actual object used by the OS.  This is
	  used for testing only. */ 
	static unsigned int SizeOfInternals(void);
	friend class osaPipeExecTest;

	int toProgram[2];
	int fromProgram[2];
	bool connected;
	bool readFlag;
	bool writeFlag;

	public:
		/* Constructor just asserts that internals size is okay */
		osaPipeExec();

		/* Destructor calls Close() */
		~osaPipeExec();

		/* Open a pipe using command and specifying read/write flags mode, which
		should be "r", "w", or "rw" specifying which direction(s) the pipe should
		go. Returns true if the Open succeeded, false otherwise */
		bool Open(const std::string & command, const std::string & mode);

		/* Close the pipe. Returns true if the Close succeeded, false
		otherwise. */
		bool Close(bool killProcess = true);

		/* Read at most maxLength characters from the pipe into buffer. Return
		number of characters read or -1 for an error */
		int Read(char * buffer, int maxLength) const;

		/* Read at most maxLength characters from the pipe, including '\0', and
		return an std::string containing those characters or empty string for
		an error */
		std::string Read(int maxLength) const;

		/* Write the null-terminated buffer to the pipe. Return the number of
		characters written or -1 for an error */
		int Write(const char * buffer);

		/* Write the the first n characters of buffer to pipe. Return the number
		of characters written or -1 for an error */
		int Write(const char * buffer, int n);

		/* Write s to the pipe and return the number of characters read or -1 for
		an error */
		int Write(const std::string & s);
};
#endif // _osaPipeExec_h
