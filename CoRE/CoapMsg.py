import struct

# http://tools.ietf.org/search/draft-ietf-core-coap-18

def enum(*sequential, **named):
    enums = dict(zip(sequential, range(len(sequential))), **named)
    return type('Enum', (), enums)

class CoapMsg:
    'the basic CoapMessage'
    
    METHOD_TYPES = { 'GET':1, 'POST':2, 'PUT':3, 'DELETE':4 }
    PACKET_TYPES = { 'CON':0, 'NON':1,  'ACK':2, 'RST':4 }
   
    #consts
    MESSAGE_ID_MIN = 0;
    MESSAGE_ID_MAX = 65535;
    COAP_MESSAGE_SIZE_MAX = 1152;
    COAP_DEFAULT_PORT = 5683;
    COAP_DEFAULT_MAX_AGE_S = 60;
    COAP_DEFAULT_MAX_AGE_MS = COAP_DEFAULT_MAX_AGE_S * 1000;

    RESPONSE_TIMEOUT_MS = 2000;
    RESPONSE_RANDOM_FACTOR = 1.5;
    MAX_RETRANSMIT = 4;
    ACK_RST_RETRANS_TIMEOUT_MS = 120000;
    ENDIAN = "<";

    def methodStr(self, mid):
        if mid==1:
            return 'GET'
        elif mid ==2 :
            return 'POST'
        elif mid ==3 :
            return 'PUT'
        elif mid ==4 :
            return 'DELETE'

    def methodId(self, methodName):
        x = self.METHOD_TYPES[methodName]
        return x
  # 0                   1                   2                   3
  # 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
  # +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  # |Ver| T |  TKL  |      Code     |          Message ID           |
  # +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  # |   Token (if any, TKL bytes) ...
  # +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  # |   Options (if any) ...
  # +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  # |1 1 1 1 1 1 1 1|    Payload (if any) ...
  # +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    # message members
    
    ver   =1
    code  = METHOD_TYPES['GET']
    msgId =0
    data1 = ""
    data2 = ""
    tokens = []
    
        
    def write(self, buf, offset):
	len1 = len(self.data1)
	len2 = len(self.data2)
	tkl  = self.tokens.__len__() & 0x07
	outputlen =0;
	# the CoAP version, type, token-len and code
	B1 = (self.ver & 0x03) | ((self.type &0x03) <<2) | (tkl <<4)
	fmt = ENDIAN + "BB"
	struct.pack_into(fmt, buf, offset +outputlen, B1, self.code & 0xff)
	outputlen += struct.calcsize(fmt)
	# the CoAP messageId
	B1 = (self.ver & 0x03) | ((self.type &0x03) <<2) | (tkl <<4)
	fmt = ENDIAN + "H"
	struct.pack_into(fmt, buf, offset +outputlen, self.msgId)
	outputlen += struct.calcsize(fmt)
	# the CoAP tokens
	fmt = ENDIAN + str(tkl) +"s"
	struct.pack_into(fmt, buf, offset +outputlen, self.tokens)
	outputlen += struct.calcsize(fmt)
	# the CoAP options
	fmt = ENDIAN + str(tkl) +"s"
	struct.pack_into(fmt, buf, offset +outputlen, self.tokens)
	outputlen += struct.calcsize(fmt)	
	return outputlen
	
    def read(self, buf, offset):
        # msg = struct.unpack("@HH10s",0,0,fTo)
        # return msg;
    	fmt = "<BBHBB"
	header_size = struct.calcsize(fmt)
	if( ( len(buf) - offset ) < header_size ):
		raise Exception("not enough data provided")
	(self.ver, self.cmd, self.cseq, len1, len2) = struct.unpack_from(fmt, buffer(buf), offset)
	if( (len(buf) - offset - header_size ) < (len1 + len2)):
		raise Exception("not enough data proviede, can't decode data area")
	fmt = "<"+str(len1)+"s"+str(len2)+"s"
	(self.data1, self.data2) = struct.unpack_from(fmt, buffer(buf), offset+ header_size )
	return header_size + len1 + len2

#
#   protected void serialize(byte[] bytes, int length, int offset){
#    	/* check length to avoid buffer overflow exceptions */
#    	this.version = 1; 
#        this.packetType = (CoapPacketType.getPacketType((bytes[offset + 0] & 0x30) >> 4)); 
#        int optionCount = bytes[offset + 0] & 0x0F;
#        this.messageCodeValue = (bytes[offset + 1] & 0xFF);
#        this.messageId = ((bytes[offset + 2] << 8) & 0xFF00) + (bytes[offset + 3] & 0xFF);		
#		
#        /* serialize options */
#        this.options = new CoapHeaderOptions(bytes, offset + HEADER_LENGTH, optionCount);
#		
#        /* get and check payload length */
#        payloadLength = length - HEADER_LENGTH - options.getDeserializedLength();
#		if (payloadLength < 0){
#			throw new IllegalStateException("Invaldid CoAP Message (payload length negative)");
#		}
#		
#		/* copy payload */
#		int payloadOffset = offset + HEADER_LENGTH + options.getDeserializedLength();
#		payload = new byte[payloadLength];
#		for (int i = 0; i < payloadLength; i++){
#			payload[i] = bytes[i + payloadOffset];
#		}
#    }
#    

m =CoapMsg()
print m.methodStr(1)
print m.methodId("POST")
# print m.serialize({'version':0x1a, 'packet_type':2, 'token_len':0, 'method':1})

buf = bytearray(256)
m.ver=2;
m.cmd=m.methodId("POST");
m.data1="safasfasf"
m.data2="934545325"
m.write(buf, 0)
print buf
m.data1=""
m.data2=""
m.read(buf, 0)
print m
    
    
