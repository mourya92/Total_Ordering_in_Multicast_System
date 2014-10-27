import random

maxMsg = raw_input('How many messages do you want to send: ')

maxNodes = raw_input('How many nodes in the distributed system: ')

mcGrp = ""


msg= ['Hello how are you', 'Hiii', 'I am fine', 'This is Mourya', 'How are you KK']

fp = open("input.txt", "w")

list=range(1,int(maxNodes))

temp_list=list

for i in range(int(maxMsg)):
	y= random.choice(temp_list)
	temp_list.remove(y)
        for j in range(random.choice(temp_list)):
                if len(temp_list)==0:
			break;
		else: 
			x= random.choice(temp_list)
			temp_list.remove(x)
			mcGrp += "P%d " % x
	temp_list=range(1,int(maxNodes))
        s = "%d P%d to %s: %s\n" % (i+1, y, mcGrp, random.choice(msg));
        mcGrp = ""
        fp.write(s);

fp.close()
