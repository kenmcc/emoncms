#imports for email
import imaplib
import smtplib
import email
from email.mime.text import MIMEText
from email.parser import HeaderParser
from email.MIMEMultipart import MIMEMultipart
from email.MIMEBase import MIMEBase
from email.Utils import COMMASPACE, formatdate
from email import Encoders
import re
import subprocess

USERNAME = "202helper@gmail.com"     #your gmail   #!/usr/bin/env python
PASSWORD = "202KRUDublin14" 			#your gmail password	

ACTIONS = ["ON", "OFF"]
SWITCHES = ["", "SADHBH_ROOM","LIGHTS","","ALL"]  
CODES = [{"ON":1298447, "OFF":1298446},
	 {"ON":1298439, "OFF":1298438},
         {"ON":1298443, "OFF":1298442},
         {"ON":1298435, "OFF":1298434},
         {"ON":1298445, "OFF":1298444}]


def check_email():
    status, email_ids = imap_server.search(None, '(UNSEEN)') #UNSEEN    #searches inbox for unseen aka unread emails ::The SEARCH command searches the mailbox for messages that match the given searching criteria.  Searching criteria consist of one or more search keys. The untagged SEARCH response from the server contains a listing of message sequence numbers corresponding to those messages that match the searching criteria. Status is result of the search command: OK - search completed, NO - search error: can't search that charset or criteria, BAD - command unknown or arguments invalid. Criteria we are using here is looking for unread emails
    if email_ids == ['']:
        print('No Unread Emails')
        mail_list = []
    else:
        mail_list = get_senders(email_ids)
        print('List of email senders: ', mail_list)         #FYI when calling the get_senders function, the email is marked as 'read'
        print len(mail_list),

    imap_server.close()
    return mail_list


	
##########################################
#FUNCTION TO SCRAPE SENDER'S EMAIL ADDRESS	
##########################################

def get_senders(email_ids):
#    print email_ids
    senders_list = []          				   #creates senders_list list 
    for e_id in email_ids[0].split():   		   #Loops IDs of a new emails created from email_ids = imap_server.search(None, '(UNSEEN)')
    	resp, data = imap_server.fetch(e_id, '(RFC822)')   #FETCH command retrieves data associated with a message in the mailbox.  The data items to be fetched can be either a single atom or a parenthesized list. Returned data are tuples of message part envelope and data.
    	perf = HeaderParser().parsestr(data[0][1])	   #parsing the headers of message
    	senders_list.append({"From":perf['From'],"Action":perf['Subject']})		   #Looks through the data parsed in "perf", extracts the "From" field 
    return senders_list

##########################################
#FUNCTION TO SEND THANK-YOU EMAILS
##########################################

def send_thankyou(mail_list, subject, body):
    for item in mail_list:
        files = []
        text = body
        msg = MIMEMultipart()
        msg['Subject'] = subject
        msg['From'] = USERNAME
        msg['To'] = item

        msg.attach ( MIMEText(text) )  

        server = smtplib.SMTP('smtp.gmail.com:587')
        server.ehlo_or_helo_if_needed()
        server.starttls()
        server.ehlo_or_helo_if_needed()
        server.login(USERNAME,PASSWORD)
        server.sendmail(USERNAME, item, msg.as_string() )
        server.quit()
        print('Email sent to ' + item + '!')

def sendHelp(From):
  body = "Available Actions: " + ",".join(ACTIONS) +"\n"
  body += "Available Switches: "+ ",".join(SWITCHES) + "\n"
  send_thankyou([From,], "Unknown Command", body)

def handleEmail(emailInfo):
  From = emailInfo['From']
  Subject = emailInfo['Action'].upper()
  pattern = re.compile("mccullagh@gmail.com")
  if not pattern.search(From):
     return
  fields = Subject.split(":")
  if len(fields) == 2:
    action = fields[0].strip()
    switch = fields[1].strip()
    if action in ACTIONS and switch in SWITCHES:
	print switch, action  
        pos = SWITCHES.index(switch)
        code = CODES[pos][action]
	resultString = "Failed"
        result = subprocess.check_call(["sudo", "./switcher", str(code), "24"])  
        if result == 0:
          resultString = "Success"
        send_thankyou([From,], "Switch Result "+ resultString, "")
    else:
	sendHelp(From)
  else:
    sendHelp(From)

imap_server = imaplib.IMAP4_SSL("imap.gmail.com",993)
imap_server.login(USERNAME, PASSWORD)
imap_server.select('INBOX')
mail_list = check_email()

for mail in mail_list:
  handleEmail(mail)

