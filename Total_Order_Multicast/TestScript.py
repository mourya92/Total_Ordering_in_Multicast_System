import sets

count=0
i=0
line_count=0
dest_count=0
source= []

dest_set = [set() for _ in xrange(50000)]
intersect_set = [set() for _ in xrange(50000)]


inter_counter=1

f = open('input.txt','r')
for line in f:
	line = line.split()
	for k in line:
		if((k!='to')):
			if(k==':'):
				break
			else:
				dest_set[line_count].add(k)
		else:
			dest_set[line_count].pop()
			dest_set[line_count].pop()
	line_count+=1

temp1= []

inter_list= []

for x in range(0 ,line_count):
	for y in range(0 ,line_count):
		if x!=y:
			if set(dest_set[x])&set(dest_set[y]):
				intersect_set[count]= set(dest_set[x]).intersection(dest_set[y])	
				temp1= list(intersect_set[count])
				if (len(temp1)==2):
					temp1.insert(0,x+1)
					temp1.insert(1,y+1)
					inter_list.append(temp1)
				count+= 1


result=1
count=0
search_count=0
string=''
final_list=[]
for each in inter_list:
	for destin in each:
		if(str(destin).isdigit()==0):
			destin=destin[1:]
			file="file"+destin+".txt"
			print "\n------------- %s----------------\n" %file
			f=open(file,'r')
			M1=each[0]
			M2=each[1]
			search1="M"+str(M1)+":"
			search2="M"+str(M2)+":"
			strings=(search1,search2)
			for each_line in f:
				if any(s in each_line for s in strings):
					each_line=each_line[:len(each_line)-1]
					string+=each_line
				search_count+=1
			search_count=0
			print string 
			final_list.append(string)
			firsttime=0
			string=''
	print final_list
	ref=final_list[0]
	for final in final_list:
		if ref==final:
			result=1
		else:
			result=0
			firsttime=1
			print "OUT OF ORDER"
			break
	if result==0:
		break

	final_list= []	
if result==1:
	print "INORDER"




