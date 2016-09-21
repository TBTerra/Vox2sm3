#include <stdio.h>
#include <string>
#include <stdlib.h>

#include "voxutils.hpp"

int main(int argc, char *argv[]){
	int i;
	struct options{
		voxutils::entType type;
		int blockID;
		int blockHP;
		voxutils::block blockD;
		std::string name;
	} opts;
	
	opts.type = voxutils::SHIP;//ship
	opts.blockID = 5;
	opts.blockHP = 100;
	
	int numNames = 0;
	for(i=1;i<argc;i++){
		if(argv[i][0]=='-'){
			switch(argv[i][1]){
				case 'a'://asteroid
					opts.type = voxutils::ASTEROID;
					break;
				case 's'://station
					opts.type =  voxutils::STATION;
					break;
				case 'b'://block stuff
					if((i+2)<argc){
						opts.blockID = atoi(argv[i+1]);
						opts.blockHP = atoi(argv[i+2]);
						i+=2;
					}else{
						printf("-b requires two arguments of block id and block hp");
						exit(EXIT_SUCCESS);
					}
					break;
				case 'h':
					printf("usage: %s [FLAGS] NAME\n",argv[0]);
					printf("NAME\tthe name of the pluprint to be saved, the file NAME.binvox will be loaded\n");
					printf("FLAGS:\n-h\tshow ith dialouge\n-a\tsave as asteroid (default:ship)\n-s\tsave as station (default:ship)\n-b ID HP\tset the block to use with the given HP value (make sure its correct)\n");
					exit(EXIT_SUCCESS);
					break;
				default:
					printf("%s is not recogniesd as a valid command switch\n",argv[i]);
					break;
			}
		}else{
			if(numNames==0){
				opts.name = argv[i];
				numNames++;
			}else{
				printf("cant have more than one file to operate on");
				exit(EXIT_FAILURE);
			}
		}
	}
	if(numNames==0){
		printf("no input file specifyed");
		exit(EXIT_FAILURE);
	}
	opts.blockD.c = opts.blockID & 0xFF;
	opts.blockD.b = ((opts.blockID >> 8) & 0x07) | ((opts.blockHP & 0x1F) << 3);
	opts.blockD.a = ((opts.blockHP >> 5) & 0x1F);
	//output the config that has been read
	printf("Bluprint name: %s\n", opts.name.c_str());
	printf("Bluprint type: %d\n",opts.type);
	printf("Block ID:%d, Block HP:%d\n",opts.blockID,opts.blockHP);
	printf("Block data [%u,%u,%u]\n", opts.blockD.a, opts.blockD.b, opts.blockD.c);
	std::string cmd = "md " +  opts.name + "\\DATA";
	system(cmd.c_str());
	cmd = "del " + opts.name + "\\DATA\\*.smd3";
	system(cmd.c_str());
	std::string loadname = opts.name + ".binvox";
	voxutils::voxObj ship = voxutils::loadBinvox(loadname);
	//voxutils::iVec3 trans = (voxutils::iVec3){16-(ship.max.x/2), 16-(ship.max.y/2), 16-(ship.max.z/2)};//move core to center
	//voxutils::iVec3 trans2 = (voxutils::iVec3){-opts.centerOff.x, -opts.centerOff.y, -opts.centerOff.z};//move core some more
	//voxutils::vox_Translate(ship, trans);
	//voxutils::vox_Translate(ship, trans2);
	voxutils::voxInfo(ship);
	voxutils::saveSM3data(ship, opts.name, opts.type, opts.blockD);
	voxutils::writeSM3extras(ship, opts.name, opts.type, 5);
	printf("done!");
	return 0;
}