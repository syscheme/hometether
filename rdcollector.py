#!/usr/bin/env python

import suds
import endpoint

RD_COL_PORT =8386
RDS_API_SOAP_URL = 'http://aliyun:8111/rdsapp/api/service?wsdl'

class HT_RDCollector(endpoint.HT_EndpointUDP):
    
    soapClient= suds.client.Client(SOAP_API_URL)
    
    def __init__(self, logger, bindEndpoint=("0.0.0.0", RD_COL_PORT)):
        endpoint.HT_EndpointUDP.__init__(self, logger, bindEndpoint)
        
    def OnResponse(self, htmsg) :
        pass

    def OnRequest(self, htreq) :
        pass

    # the requests to RDS
    def registerVolume(self, params):
        # soapClient.service.APIXXXX(...)
        pass
    
    def registerResource(self, moreparams) :
        pass
    
    def updateResource(self, uri, params) :
        pass

    def OnIdle(self) :
        print "\r listening..."
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
import sys
import logging

logger = logging.getLogger("endlesscode")
formatter = logging.Formatter('%(name)-12s %(asctime)s %(levelname)-8s %(message)s', '%a, %d %b %Y %H:%M:%S',)
file_handler = logging.FileHandler("test.log")
file_handler.setFormatter(formatter)  
stream_handler = logging.StreamHandler(sys.stderr)  
logger.addHandler(file_handler)  
logger.addHandler(stream_handler)
logger.setLevel(logging.DEBUG)

ep = HT_RDCollector(logger)
#parser = HT_MessageProto(ep)
#msg="POST / HTTP/1.1\r\raaa:bbb\r\n\r\n\r\nbody"
#htmsg = parser.parse(msg, "")
#msg = ep.sendRequest(htmsg, ("localhost",8112))

ep.start()
sleep(20)
ep.stop()

msg='</sensors/temp>;if=\"sensor\" ,\n\n</sensors/light>;if=\" sensor \"'
print rs.readCoRELinks(msg)

print "program exits"



    

