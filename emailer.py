# Import smtplib for the actual sending function
import smtplib

# Import the email modules we'll need
from email.mime.text import MIMEText


def sendEmail(Subject, Msg, To, From="weatherlogger@home.ie"):
  msg = MIMEText(Msg)
  me = From
  you = To
  msg['Subject'] = Subject
  msg['From'] = me
  msg['To'] = you

# Send the message via our own SMTP server, but don't include the
# envelope header.
  s = smtplib.SMTP('localhost')
  s.sendmail(me, [you], msg.as_string())
  s.quit()
