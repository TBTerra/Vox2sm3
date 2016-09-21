#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <vector>
#include <string>
#include <zlib.h>
#include <math.h>

namespace voxutils{
	enum entType{
		SHIP = 0,
		SHOP = 1,
		STATION = 2,
		ASTEROID = 3,
		PLANET = 4
	};
	struct fileReserve{
		char* name=NULL;
		FILE* fp=NULL;
		fileReserve(){}
	};
	struct iVec3{
		int32_t x;
		int32_t y;
		int32_t z;
	};
	struct voxObj{
		iVec3 min;
		iVec3 max;
		uint32_t numVox;
		std::vector<uint8_t> data;
		//data order is increasing y (up) fastest, z (forwards) next, x (right when veiwed from the front, left from the pilots view) last
	};
	struct block{
		uint8_t a;
		uint8_t b;
		uint8_t c;
	};
	struct segment{
		uint8_t version;
		uint8_t timestamp[8];
		uint8_t x[4];
		uint8_t y[4];
		uint8_t z[4];
		uint8_t dataByte;
		uint8_t compSize[4];
		uint8_t data[49126];
	};
	
	voxObj loadBinvox(std::string &filename){
		printf("Loading %s\n",filename.c_str());
		FILE* inF = fopen(filename.c_str(),"rb");
		uint32_t version;
		iVec3 dim;
		float tran[3];
		float scale;
		fscanf(inF, "#binvox %u\n", &version);
		fscanf(inF, "dim %u %u %u\n", &dim.x, &dim.z, &dim.y);//swap z and y dimension sizes
		fscanf(inF, "translate %f %f %f\n", &tran[0], &tran[1], &tran[2]);
		fscanf(inF, "scale %f\ndata\n", &scale);
		voxObj ret;
		ret.numVox = 0;
		ret.min.x = ret.min.y = ret.min.z = 0;
		ret.max.x = dim.x - 1;
		ret.max.y = dim.y - 1;
		ret.max.z = dim.z - 1;
		
		//calculate the reverse transform to work out translation
		uint32_t maxD = dim.x;
		if(maxD < dim.y)maxD = dim.y;
		if(maxD < dim.z)maxD = dim.z;
		
		double Tx = ((double)maxD)*((double)tran[0]/(double)scale);
		double Ty = ((double)maxD)*((double)tran[1]/(double)scale);
		double Tz = ((double)maxD)*((double)tran[2]/(double)scale);
		
		ret.min.x += 16 + (int)floor(Tx);
		ret.max.x += 16 + (int)floor(Tx);
		ret.min.y += 16 + (int)floor(Ty);
		ret.max.y += 16 + (int)floor(Ty);
		ret.min.z += 16 + (int)floor(Tz);
		ret.max.z += 16 + (int)floor(Tz);
		
		uint32_t done = 0;
		uint32_t targ = dim.x * dim.y * dim.z;
		printf("using %.2fMiB of memory for block array\n", (double)targ/1048576.0);
		ret.data.reserve(targ);
		uint8_t entry[2];
		while(done<targ){
			fread(entry,1,2,inF);
			if(entry[0])ret.numVox += entry[1];
			while(entry[1]){
				ret.data[done] = entry[0];
				entry[1]--;
				done++;
			}
		}
		return ret;
	}
	
	void vox_Translate(voxObj &vox, iVec3 trans){
		vox.min.x+=trans.x;
		vox.max.x+=trans.x;
		vox.min.y+=trans.y;
		vox.max.y+=trans.y;
		vox.min.z+=trans.z;
		vox.max.z+=trans.z;
	}
	
	uint8_t vox_getBlock(voxObj &vox, iVec3 pos){
		if(pos.x < vox.min.x)return 0;
		if(pos.x > vox.max.x)return 0;
		if(pos.y < vox.min.y)return 0;
		if(pos.y > vox.max.y)return 0;
		if(pos.z < vox.min.z)return 0;
		if(pos.z > vox.max.z)return 0;
		//determine what block to return
		pos.x -= vox.min.x;
		pos.y -= vox.min.y;
		pos.z -= vox.min.z;
		int h = (vox.max.y - vox.min.y) + 1;
		int lh = h * ((vox.max.z - vox.min.z) + 1);
		unsigned int index = pos.x * lh + pos.z * h + pos.y;
		return vox.data[index];
	}
	
	void genSM3section(voxObj &vox, block* blok, iVec3 offset, entType type, block there){
		int i,j,k;
		for(i=0;i<32;i++){//x
			for(k=0;k<32;k++){//z
				for(j=0;j<32;j++){//y
					if((type==SHIP)&&((offset.x+i)==16)&&((offset.y+j)==16)&&((offset.z+k)==16)){
						blok[i+32*j+1024*k] = (block){0x07,0xD0,0x01};//ship core
						continue;
					}
					if(vox_getBlock(vox, (iVec3){offset.x+i, offset.y+j, offset.z+k})){
						blok[i+32*j+1024*k] = there;//grey standard armor
					}else{
						blok[i+32*j+1024*k] = (block){0,0,0};//blank
					}
				}
			}
		}
	}

	int32_t div512(int32_t a){//divide that always floors return, not round towards 0
		if(a>=0){
			return a/512;
		}else{
			return (a-511)/512;
		}
	}
	
	int32_t floor32(int32_t a){//round down to nearest factor of 32 (including negatives)
		if(a>=0){
			return (a/32)*32;
		}else{
			return ((a-31)/32)*32;
		}
	}
	
	void saveSM3section(block* blok, std::string &name, iVec3 offset){
		static std::string curName = "Null";
		static FILE* fp = NULL;
		iVec3 regionoff = (iVec3){div512(offset.x+256),div512(offset.y+256),div512(offset.z+256)};
		iVec3 inregionoff = (iVec3){
			((offset.x-(regionoff.x*512))+256)/32,
			((offset.y-(regionoff.y*512))+256)/32,
			((offset.z-(regionoff.z*512))+256)/32
		};
		char extention[80];
		sprintf(extention, ".%d.%d.%d.smd3", regionoff.x,regionoff.y,regionoff.z);
		std::string trueName = name + "/DATA/" + name + extention;
		//printf("region %s\n",trueName.c_str());
		//printf("cur file %s\n",curName.c_str());
		//printf("section %d,%d,%d\n",inregionoff.x,inregionoff.y,inregionoff.z);
		//load file of region
		if(trueName != curName){//need to open different file
			printf(";");
			fclose(fp);
			if((fp = fopen(trueName.c_str(), "r+b"))==NULL){
				fclose(fp);
				printf(":");
				//need new file
				fp = fopen(trueName.c_str(), "wb");
				uint8_t version[4] = {2,0,0,0};
				fwrite(version, 1, 4, fp);
				version[0]=0;
				int i;
				for(i=0;i<4096;i++){
					fwrite(version, 1, 4, fp);
				}
				//reopen in proper
				fp = freopen(trueName.c_str(), "r+b", fp);
			}
			curName = trueName;
		}
		//now we have a valid file (probably) we need to work out where to write
		uint32_t indexIndex = (1+inregionoff.x+(16*inregionoff.y)+(256*inregionoff.z))*4;
		//printf("index at %u\n",indexIndex);
		fseek(fp, indexIndex, SEEK_SET);
		uint8_t entry[4];
		fread(entry,1,4,fp);
		uint32_t index;
		if(entry[0]||entry[1]){
			//section exists already
			index = (((uint32_t)entry[0]<<8) + entry[1])*49152 - 32764;
		}else{
			//section does not exist yet
			fseek(fp,0,SEEK_END);
			index = ftell(fp);
		}
		//printf("data at %u\n",index);
		//make the payload
		segment seg;
		
		int i;
		for(i=0;i<49152;i++){((uint8_t*)&seg)[i]=0;}
		uLongf destL = 49126;
		int compG = compress(seg.data, &destL, (Bytef*)blok, 3*32768);
		//printf("data compressed to %u", destL);
		if(compG != Z_OK){
			printf("error compressing section data\n");
			return;
		}
		for(i=destL;i<49126;i++){seg.data[i]=0;}
		seg.compSize[3] = destL&0xFF;
		seg.compSize[2] = (destL>>8)&0xFF;
		seg.compSize[1] = 0;
		seg.compSize[0] = 0;
		seg.dataByte = 1;
		seg.version = 2;
		for(i=0;i<4;i++){
			seg.x[3-i] = (offset.x>>(8*i))&0xFF;
			seg.y[3-i] = (offset.y>>(8*i))&0xFF;
			seg.z[3-i] = (offset.z>>(8*i))&0xFF;
		}
		//add time-stamp eventually (currently 1st jan 1970)
		//save the payload
		fseek(fp, index, SEEK_SET);
		fwrite(&seg,1,sizeof(segment),fp);
		//update the index
		fseek(fp, indexIndex, SEEK_SET);
		index = (index/49152)+1;
		destL += 26;
		entry[1] = index&0xFF;
		entry[0] = (index>>8)&0xFF;
		entry[3] = destL&0xFF;
		entry[2] = (destL>>8)&0xFF;
		fwrite(entry,1,4,fp);
		fflush(fp);
	}

	void saveSM3data(voxObj &vox, std::string &name, entType type, block blok){
		int minX = floor32(vox.min.x);
		int minY = floor32(vox.min.y);
		int minZ = floor32(vox.min.z);
		int maxX = floor32(vox.max.x)+31;
		int maxY = floor32(vox.max.y)+31;
		int maxZ = floor32(vox.max.z)+31;
		printf("printing output data files\n");
		printf("output bounds [%d,%d,%d]-[%d,%d,%d]\n",minX,minY,minZ,maxX,maxY,maxZ);
		int i=0,j=0,k=0;
		block rawData[32764];
		for(i=minX;i<maxX;i+=32){
			for(k=minZ;k<maxZ;k+=32){
				for(j=minY;j<maxY;j+=32){
					//printf("outputting %d,%d,%d",i,j,k);
					genSM3section(vox, rawData, (iVec3){i, j, k}, type, blok);
					//check that its not blank
					uint8_t* tesPtr = (uint8_t*)rawData;
					int l,pass = 0;
					for(l=0;l<(3*32*32*32);l++){
						if(tesPtr[l])pass=1;
					}
					if(!pass){printf(".");continue;}
					
					//FILE* testF=fopen("sample.dat","wb");
					//fwrite(rawData,1,3*32*32*32,testF);
					//fclose(testF);
					saveSM3section(rawData, name, (iVec3){i, j, k});
					printf(",");
				}
			}
		}
		printf("\n");
	}
	
	void reverseF(float val, uint8_t* ptr){
		uint8_t* valP = (uint8_t*)&val;
		ptr[3]=valP[0];
		ptr[2]=valP[1];
		ptr[1]=valP[2];
		ptr[0]=valP[3];
	}
	
	void writeSM3extras(voxObj &vox, std::string &name, entType type, int id){
		std::string filename = name + "\\meta.smbpm";
		printf("writing meta file\n");
		FILE* fp = fopen(filename.c_str(),"wb");
		uint8_t meta[] = {0,0,0,0,1};
		fwrite(meta,1,5,fp);

		filename = name + "\\logic.smbpl";
		printf("writing logic file\n");
		fp = freopen(filename.c_str(),"wb",fp);
		uint8_t logic[] = {0,0,0,0,255,255,251,254,0,0,0,0};
		fwrite(logic,1,12,fp);

		filename = name + "\\header.smbph";
		printf("writing header file\n");
		fp = freopen(filename.c_str(),"wb",fp);
		uint8_t header[] = {0,0,0,2,0,0,0,type};
		fwrite(header,1,8,fp);
		reverseF((float)vox.min.x-16,header);
		fwrite(header,1,4,fp);
		reverseF((float)vox.min.y-16,header);
		fwrite(header,1,4,fp);
		reverseF((float)vox.min.z-16,header);
		fwrite(header,1,4,fp);
		reverseF((float)vox.max.x-16,header);
		fwrite(header,1,4,fp);
		reverseF((float)vox.max.y-16,header);
		fwrite(header,1,4,fp);
		reverseF((float)vox.max.z-16,header);
		fwrite(header,1,4,fp);
		header[0]=0;header[1]=0;header[2]=0;header[3]=2;//number of block types
		fwrite(header,1,4,fp);
		header[1]=1;header[3]=0;header[5]=1;//core
		fwrite(header,1,6,fp);
		header[0]=(id>>8)&0xFF;
		header[1]=id&0xFF;
		if(vox_getBlock(vox, (iVec3){16,16,16})){vox.numVox--;}
		header[5]=vox.numVox&0xFF;
		header[4]=(vox.numVox>>8)&0xFF;
		header[3]=(vox.numVox>>16)&0xFF;
		header[2]=(vox.numVox>>24)&0xFF;//block type
		fwrite(header,1,6,fp);
		fwrite(header,1,1,fp);//end of data (not including the new stuff
		fclose(fp);
	}
	
	void voxInfo(voxObj &vox){
		printf("bounding box [%d,%d,%d]-[%d,%d,%d]\n",
			vox.min.x,vox.min.y,vox.min.z,
			vox.max.x,vox.max.y,vox.max.z);
		printf("total of %u blocks\n",vox.numVox);
	}
}