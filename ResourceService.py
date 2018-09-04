#!/usr/bin/env python

import threading
from time import sleep, ctime
import socket
import time
import select
import re
import string
import datetime

RESOURCE_SVC_PORT =8386

class TimeoutException(Exception):
    pass

class HT_Message (object):
    def __init__(self, proto, peer):
        self.command=dict()
        self.headers=dict()
        self.content_body =""
        self.cSeq =0
        self.stampCreated = datetime.time()
        self.proto = proto
        self.peer = peer

class HT_Request(HT_Message):
    def __init__(self, localEP, proto, peer):
        HT_Message.__init__(self, proto, peer)
        self.localEP = localEP

class HT_EndpointUDP (threading.Thread):
    def __init__(self, bindEndpoint=("0.0.0.0", RESOURCE_SVC_PORT)):
        threading.Thread.__init__(self)
        self.bindEndpoint  = bindEndpoint
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM, 0)
        self.sock.bind(self.bindEndpoint)
        self.bQuit = False
        self.lastCSeq = 0
        
    def __del__(self) :
        self.stop()
        
    def nextCSeq(self) :
        return ++self.lastCSeq
        
    def stop(self) :
        self.bQuit = True
        self.sock.close()
          
    def run(self) : # the thread covers the receiving
        while not self.bQuit :
            rlist = [ self.sock ]
            xlist = [ self.sock ]
            (rl,_,xl) = select.select(rlist, [], xlist, 2)
            if( self.sock in xl):
                print "socket error, quit thread"
                break;
            
            if( not self.sock in rl) :
                continue;
            
            if (self.bQuit) :
                break;
            
            #this is an incomming message, receive/parse and dispatch it
            (msg, peer) = self.sock.recvfrom(1)
            self.handleMessage(msg, peer)
                 
    def handleMessage(self, msg, peer) :
        m = re.match("^(\d+)[\s]+([^\r\n]+)[\r\n]", msg) # the start line
        if m is not None : # this is a response
            htmsg = HT_Message("HTTP", peer)
            htmsg.command['verb'] = 'RESP'
            htmsg.command['status_code'] = m.group(1)
            htmsg.command['status_msg']  = m.group(2)
        else :
            m = re.match("^([^\s]+)[\s]+([^\s]+)\s*HTTP\/[^\r\n]*[\r\n]{1,2}", msg) # the start line
            if m is not None : # this is a request
                htmsg = HT_Request(self, "HTTP", peer)
                htmsg.command['verb'] = m.group(1)
                htmsg.command['uri']  = m.group(2)
                
        if htmsg is None: # illegal message
            return
            
        msg = msg[m.end():]
        # the header lines
        while True :
            m = re.match("^([^\s:]+)[\s]*:[\s]*([^\r\n]*)[\r\n]{1,2}", msg)
            if m is None : # header lines end
                break;
            htmsg.headers[m.group(1)] = m.group(2)
            msg = msg[m.end():]
                
        # skip the empty line
        m = re.match("^[\r\n]{1,2}", msg)
        if m is not None :
            msg = msg[m.end():]
                
        # the HTTP content body
        htmsg.content_body = msg;
            
        # dispatch the message
        if 'RESP' == htmsg.command['verb'] :
            return self.OnResponse(htmsg)
        elif 'POST' == htmsg.command['verb'] :
            return self.OnPOST(htmsg)
            pass
            
    def readCoRELinks(self, linktext):
        # parse links according to http://tools.ietf.org/html/rfc6690
        # needed when Content-Type = application/link-format
        linkstrs= string.split(linktext, ',')
        links =list()
        for line in linkstrs:
            linkparams = dict()
            line = line.strip();
        
            # the uri
            m = re.match("[^<]*<([^>]+)>;*", line)
            if m is None:
                continue

            uri = m.group(1).strip('\"\t\n\r ')
            line = line[m.end():]
        
            #link params
            while True :
                m = re.match("\s*([^\s=]+)\s*=([^;]+);*", line)
                if m is None : # header lines end
                    break;
                k = m.group(1)
                v = m.group(2)
                linkparams[k.strip('\"\t\n\r ')] = v.strip('\"\t\n\r ')
                line = line[m.end():]
            
            links.append({'uri': uri, 'params': linkparams })
        
        return links
        
    def OnResponse(self, response) :
        pass
    
    def OnPOST(self, message) :
        pass

    def OnGET(self, uri, querystr) :
        pass
        
    def OnPUT(self, uri, querystr) :
        pass
        
    # the requests to RDS
    def registerResource(self, moreparams) :
        pass
    
    def updateResource(self, uri, params) :
        pass


class HTGw(threading.Thread) :
    def __init__(self, rdsEndpoint=("rds.hometehter.com", RESOURCE_SVC_PORT), bindEndpoint=("0.0.0.0",0)):
        self.rdsEndpoint    = rdsEndpoint
        self.bindEndpoint   = bindEndpoint
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM, 0)
        self.sock.bind(self.bindEndpoint)
        
    def run(self) : # the thread covers the receiving
        while not self.quit :
            rlist = [ self.sock ]
            xlist = [ self.sock ]
            (rl,_,xl) = select.select(rlist, [], xlist, timeout)
            if( self.sock in xl):
                print "socket error, quit thread"
                sys.exit(-1)
            
            if( not self.sock in rl) :
                continue;
            
            #this is an incomming message, receive/parse and dispatch it
            (msg, peer) = self.sock.recvfrom(10)
            m = re.match("POST[\s]+([^\s]+)[^\r\n]*[\r\n]Endpoint:[\s]*([^\s]*)", msg)
            if m is not None :
                OnPOST(self, msg)
                
                
        
    def OnResponse(self, response) :
        pass
    
    def OnGET(self, uri, querystr) :
        pass
        
    def OnPUT(self, uri, querystr) :
        pass
        
    def OnPOST(self, uri, querystr) :
        pass
        
    # the requests to RDS
    def registerResource(self, moreparams) :
        pass
    
    def updateResource(self, uri, params) :
        pass
        

def loop0() :
    while True:
        print "L0 at :", ctime()
        sleep(2)
        
def loop1() :
    while True:
        print "L1 at :", ctime()
        sleep(3)
        
#thread.start_new_thread(loop0, ())
#thread.start_new_thread(loop1, ())

#sleep(60)

rs = HT_EndpointUDP()
msg="POST / HTTP/1.1\r\raaa:bbb\r\n\r\n\r\nbody"
rs.handleMessage(msg, "")
#rs.start()
sleep(20)
rs.stop()

msg='</sensors/temp>;if=\"sensor\" ,\n\n</sensors/light>;if=\" sensor \"'
print rs.readCoRELinks(msg)

print "program exits"


