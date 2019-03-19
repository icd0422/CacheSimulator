#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <cstring>
#include <cmath>
#include <stack>
using namespace std;

typedef struct {
	int tag;
	int valid;
}Line;

class LRU{
private:
	int size;
	int* RU;	//MRUbit�� RU[0], LRUbit�� RU[size-1]
	int associativity;
public:
	LRU() {}
	void init(int associativity) {	//LRU �ʱ�ȭ
		size=0;
		RU = new int[associativity];
		this->associativity = associativity;
	}
	void put(int num) {
		int temp;
		int state = 1;
		for (int i = 0; i < size; i++) {		//�ֱ� ����� slot�� ���� ������..
			if (num == RU[i]) {
				state = 0;
				temp = RU[i];
				for (int j = i; j > 0; j--) {
					RU[j] = RU[j - 1];
				}
				RU[0] = num;
				break;
			}
		}
		if (size == associativity) {
			//������ ��ġ���� ����
		}
		else if(state) {
			for (int i = size; i > 0; i--) {
				RU[i]=RU[i-1];
			}
			RU[0] = num;
			size++;
		}
	}
	int getLRUbit() {
		return RU[size-1];
	}
	int getSize() {
		return size;
	}
};

typedef struct {
	Line* myLine;
	LRU* lru;
}Set;

class Cache{
private:
	int totalbyte;		//cache�� ��ü ����Ʈ ���� ũ�� 
	int blockbyte;		//cache block �ϳ��� ����Ʈ ���� ũ�� 
	int nSet;			//set�� ����	2�� index��
	int associativity;	//associativity,�� N-way associative cache���� N��
	int nTag;			//TIO������ ��Ʈ
	int nIndex;
	int nOffset;		
	int hit;			//����, ��Ʈ, �̽� ī��Ʈ
	int miss;
	int access;
	Set* allSet;		//Set����ü �迭�� ������ �ִ� ����
	Set* mySet;			//Set ����ü�� ����Ű�� ������ ����, ���� ������ ����..
public:
	Cache() {}
	~Cache() {
		for(int i=0; i<nSet; i++) {		//�Ҹ��ڿ��� �޸� ��ȯ
			mySet = allSet +i;
			free(mySet->myLine);
			free(mySet->lru);
		}
		free(allSet);

	}
	int init(int TB, int BB, int A) {					//�־��� �������� ������ �ʱ�ȭ �ϴ� �Լ�
		totalbyte = TB;
		blockbyte = BB;
		associativity = A;
		nSet = (totalbyte/blockbyte)/associativity;

		nOffset = log((double)blockbyte)/log(2.0);			//������ ���� TIO��Ʈ����
		nIndex = log((double)totalbyte)/log(2.0) - log((double)associativity)/log(2.0) - nOffset;
		nTag = 32 - nOffset - nIndex;
		hit=0;
		miss=0;
		access=0;

		allSet = (Set*)malloc((nSet)*sizeof(Set));	
		//set����ü�� nSet��ŭ(�ε����ǰ���) ������ ���� ����. allSet[nSet]
		for(int i=0; i<nSet; i++) {			//������ set����ü �ʱ�ȭ
			mySet = allSet+i;
			mySet->myLine = (Line*)malloc((associativity)*sizeof(Line));	
			//Line����ü�� associativity��ŭ(slot�ǰ���) ������ ���� ����

			for(int j=0; j<associativity; j++) {		//��Line�ʱ�ȭ
				(mySet->myLine+j)->tag =-1;
				(mySet->myLine+j)->valid =0;
			}
			mySet->lru= new LRU();						//LRU�ʱ�ȭ
			(mySet->lru)->init(associativity);
		}

		if(log((double)blockbyte)/log(2.0) - (int)(log((double)blockbyte)/log(2.0)) != 0 ){
			printf("Cache block�� ũ�Ⱑ 2�� �������� �ƴ� ��� \n");
			return 1;
		}
		if(associativity !=1 && associativity !=2 && associativity != 4 && associativity != 8) {
			printf("Associativity�� 1, 2, 4, 8 �̿��� ���� �Էµ� ��� \n");
			return 1;
		}
		if(totalbyte % (blockbyte * associativity) != 0) {
			printf("Cache ��ü�� ũ�Ⱑ (cache block ũ��)x(associativity)�� ����� �ƴ� ��� \n");
			return 1;
		}
		return 0;
	}
	void printTIO() {									//TIO��Ʈ�� ����ϴ� �Լ�
		printf("tag: %d bits \nindex: %d bits \noffset: %d bits\n", nTag, nIndex, nOffset);
	}
	void printfHitRate() {	//�ùķ������� ��� ��Ʈ����Ʈ�� ����ϴ� �Լ�
		printf("Result: total access %d, hit %d, hit rate %.2f\n",
			access, hit, (float)hit/access);
	}
	int Simulator(const char* traceFile) {				
		//�־��� ĳ���� ���¿� ���� �ùķ����͸� ������ �Լ�
		char none[256];
		int address=0;
		FILE* fin = NULL;
		if((fin = fopen(traceFile, "r"))== NULL ) {					//������ �б���� ����
			printf("�������� ���� Ʈ���̽� ���� �Է����� ��� \n");	//������ ������� ����1
			return 1;
		}
		int tag, index, offset;							//���� ������ ����	
		int tagMask, indexMask, offsetMask;				//��Ʈ����ũ

		offsetMask = blockbyte-1;						
		//ex ���� blockbyte == 16�̶�� 0000 0000 0000 0000 0000 0000 0000 1111
		indexMask = (nSet-1)<<nOffset;	
		tagMask = -1<<(nIndex+nOffset);

		while(!feof(fin)) {													//������ ������ ����
			if(fscanf(fin, "%s %x %s", none, &address, none) != 3) break;	//\n\0���� \0�� ������� ����

			offset = address & offsetMask;									//��Ʈ����ũ�� �� ���� ����
			index = (address & indexMask)>>nOffset;
			tag = (address & tagMask)>>nIndex+nOffset;

			access++;									//����++
			//printf("@tag : %x,\tindex : %x,\t",tag, index);
			mySet = allSet + index;						// mySet = allSet[index]
			bool missOrHit = false;						//false�� miss, true�� hit
			for(int j=0; j<associativity; j++) {
				if((mySet->myLine)->valid == 1 && (mySet->myLine+j)->tag == tag) {		
					//������ ä���� �ְ� �����±װ� �ִٸ�
					missOrHit = true;
					hit++;								//��Ʈ++
					//cout<<"hit,\tslot : "<< j<<endl;
					(mySet->lru)->put(j);				//LRU���
					break;
				}
			}
			if(missOrHit ==false) {					//�̽����
				miss++;								//�̽�++
				
				int size = (mySet->lru)->getSize();	
				//size�� �������� invalue�� üũ�ؼ� ���Ҽ��� ������,, 
				//LRU����ü�� �̿��Ͽ� ����.
				if(size == associativity) {	
					//�����ִٸ� LRU��Ʈ�� ���� ä�� �ִ´�.
					int LRUbit = (mySet->lru)->getLRUbit();	
					(mySet->myLine+LRUbit)->tag=tag;
					(mySet->myLine+LRUbit)->valid=1;
					(mySet->lru)->put(LRUbit);	//LRU���
					//cout<<"miss,\tslot : "<<LRUbit<<endl;
				}else {	
					//������ �ʾҴٸ� ���� ĭ�� ���� ä�� �ִ´�.
					(mySet->myLine+size)->tag=tag;
					(mySet->myLine+size)->valid=1;	
					(mySet->lru)->put(size);	//LRU���
					//cout<<"miss,\tslot : "<<size<<endl;
				}
			}
		}
		fclose(fin);				//������ �ݴ´�.
		return 0;
	}
};

int main(int argc, const char* argv[]) {

	if(argc != 5) {
		printf("���α׷��� �Է� ���ڰ� 4���� �ƴ� ���\n");				//�Էµ� ������ ���� �߸��� ���
		printf("����. simple_cache_sim <trace file> <cache's byte size> <cache block's byte size> <number of associativity>\n");
		return 1;
	}
	Cache* cache = new Cache();
	const char* traceFile = argv[1];

	if(cache->init(atoi(argv[2]),atoi(argv[3]),atoi(argv[4])) == 0) {	
		//�Էµ� ���� �ʱ�ȭ�� ������ ���
		cache->printTIO();
		if(cache->Simulator(traceFile) == 0) {				
			//�ùķ����Ͱ� ���� ������� ���
			cache->printfHitRate();
		}else 
			printf("�ùķ������� ������ ����");
	}else 
		printf("�ùٸ��� ���� ĳ���� �ʱ�ȭ\n");

	return 0;
}
