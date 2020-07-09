# sudo python3 ./send.py

from scapy.all import *

#### Required for version 2.4.3 ####
# load_contrib("automotive.someip")
# load_contrib("automotive.someip_sd")
#
# i = IP(src="192.168.0.4", dst="192.168.0.3")
# u = UDP(sport=48664, dport=30509)
# sip = SOMEIP()
# sip.iface_ver = 0
# sip.proto_ver = 1
# sip.msg_type = "REQUEST"
# sip.retcode = "E_OK"
# sip.msg_id.srv_id = 0x1234
# sip.msg_id.sub_id = 0x0
# sip.msg_id.method_id=0x0421
# sip.req_id.client_id = 0x1313
# sip.req_id.session_id = 0x0010
# sip.add_payload(Raw ("Hello!"))
# p = i/u/sip
# res = sr1(p)

#### Required for version 2.4.3-dev699 (master) ####
load_contrib("automotive.someip")

i = IP(src="192.168.0.4", dst="192.168.0.3")
u = UDP(sport=48664, dport=30509)
sip = SOMEIP()
sip.iface_ver = 0
sip.proto_ver = 1
sip.msg_type = "REQUEST"
sip.retcode = "E_OK"
sip.srv_id = 0x1234
sip.sub_id = 0x0
sip.method_id=0x0421
sip.client_id = 0x1313
sip.session_id = 0x0010
sip.add_payload(Raw ("ping"))
p = i/u/sip
res = sr1(p)
