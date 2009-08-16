#!/usr/bin/env python
#
# Print the build information embedded in a header file.
#
# Expects to be invoked from the command line with a file name and a
# list of directories to search.  The file name will be one of the
# following:
#
#   llversionserver.h
#   llversionviewer.h
#
# The directory list that follows will include indra/llcommon, where
# these files live.

import errno, os, re

def svnInfo(file):
    pattern = re.compile("Last Changed Rev: (\w+)")
    for line in file:
	#print >> sys.stderr, "build...: \"%s\"" % (line)
	match = pattern.search(line)
	if(match):
	    file.close()
	    return match.group(1)
    file.close()
    return None

def get_version(filename):
    #print >> sys.stderr, "cwd...: \"%s\"" % ( os.getcwd() )
    fp = open(filename)
    data = fp.read()
    fp.close()

    vals = {}
    m = re.search('const S32 LL_VERSION_MAJOR = (\d+);', data)
    vals['major'] = m.group(1)
    m = re.search('const S32 LL_VERSION_MINOR = (\d+);', data)
    vals['minor'] = m.group(1)
    m = re.search('const S32 LL_VERSION_PATCH = (\d+);', data)
    vals['patch'] = m.group(1)
    vals['build'] = None
    try:
	stdin = os.popen("svn info")
	vals['build'] = svnInfo(stdin)
	print "svn:", vals['build']
    except ValueError, IOError:
	pass
    if not vals['build']:
	try:
	    stdin = os.popen("git svn info")
	    vals['build'] = svnInfo(stdin)
	    print "git", vals['build']
	except ValueError, IOError:
	    pass
    if not vals['build']:
	m = re.search('const S32 LL_VERSION_BUILD = (\d+);', data)
	vals['build'] = m.group(1)
	print "grep:", vals['build']

    return "%(major)s.%(minor)s.%(patch)s.%(build)s" % vals

if __name__ == '__main__':
    import sys

    try:
        for path in sys.argv[2:]:
            name = os.path.join(path, sys.argv[1])
            try:
                print get_version(name)
                break
            except OSError, err:
                if err.errno != errno.ENOENT:
                    raise
        else:
            print >> sys.stderr, 'File not found:', sys.argv[1]
            sys.exit(1)
    except AttributeError:
        print >> sys.stderr, 'Error: malformatted file: ', name
        sys.exit(1)
    except IndexError:
        print >> sys.stderr, ('Usage: %s llversion[...].h [directories]' %
                              sys.argv[0])
