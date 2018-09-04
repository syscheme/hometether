#!/usr/bin/env python

import re
import string
import datetime

class ResourceData(object):
    def __init__(self, path, name, ivalue ={}, isLoaded=False):
        self.path = path
        self.name = name
        self.ivalue = dict(ivalue)
        if isLoaded:
            self.stampLoaded = datetime.time()
        else: self.stampLoaded =0
        self.stampModified =0
    
    def __del__(self):
        if self.stampModified > self.stampLoaded :
            self.save()
    
    def flush(self):
        pass
    
    def setInts(self, ivalues) :
        self.ivalues = ivalues
        self.stampModified = datetime.time()
    
class ResourcePath(object):
    def __init__(self, volume, pathname):
        self.volume = volume
        self.name = pathname
        self.resMap = dict()
        
    @staticmethod
    def readCoRELinks(linktext):
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
        
    def importCoRELinks(self, links) :
        pass
    

