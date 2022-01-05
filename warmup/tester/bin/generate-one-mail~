#!/usr/bin/python

import sys, os, smtplib
from email.MIMEMultipart import MIMEMultipart
from email.MIMEBase import MIMEBase
from email.MIMEText import MIMEText
from email.Utils import COMMASPACE, formatdate
from email import Encoders
from subprocess import Popen, PIPE
import mailbox
import email.utils
import os.path
from optparse import OptionParser

def generate_mail(TA, email, msgtextfile, assignment, files, grp):
    msg = MIMEMultipart()
    msg['From'] = TA
    msg['To'] = email
    msg['Subject'] = 'ECE344: Group %s, marker results for assignment %s'%(
        grp, assignment)
    try:
        fd = open(msgtextfile, "r")
        msg.attach(MIMEText(fd.read()))
        fd.close()
    except IOError:
        print 'File ' + msgtextfile + ' not found'
        return None

    for f in files:
        try:
            part = MIMEBase('application', "octet-stream")
            fd = open(f, "rb")
            part.set_payload(fd.read())
            Encoders.encode_base64(part)
            part.add_header('Content-Disposition',
                            'attachment; filename="%s"' % os.path.basename(f))
            msg.attach(part)
            fd.close()
        except IOError:
            print 'File ' + f + ' not found'
    return msg

def generate_mbox(TA, email, msgtextfile, assignment, grp):
	mbox = mailbox.mbox("mail-" + assignment + ".mbox")
	mbox.lock()
	try:
            files = ["marker-" + grp + ".log",
                     "tester-" + grp + ".log",
                     "tester-" + grp + ".out"]
            msg = generate_mail(TA, email, msgtextfile, assignment, files, grp)
            if msg is not None:
                mbox.add(msg)
                mbox.flush()
	finally:
            mbox.unlock()
	return

def main():
    parser = OptionParser()
    parser.add_option("-t", dest = "TA", help = "TA information")
    parser.add_option("-m", dest = "email", help = "student emails")
    parser.add_option("-f", dest = "file", help = "mail txt file")
    parser.add_option("-a", dest = "asst", help = "assignment number")
    parser.add_option("-g", dest = "grp", help = "group number")
    (options, args) = parser.parse_args(sys.argv)
    if options.TA is None or options.email is None or options.file is None or \
            options.asst is None or options.grp is None:
        parser.print_help()
        exit()

    generate_mbox(options.TA, options.email, options.file, options.asst, 
                  options.grp)

if __name__ == "__main__":
	main()
