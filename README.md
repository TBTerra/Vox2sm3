# Vox2sm3 V0.3.3
Binvox to Starmade blueprint converter for the importing of 3d models

### What is Vox2sm3
Vox2sm3 is a command-line tool for converting .binvox voxel files into blueprint files for Starmade, combined with the binvox program, it allows the importing of shells of entire ships from their 3dmodel.
### Features
* Pickable block type to use
* when exporting a ship, a core is placed where the user specifies
* can export as a ship, a station and as an asteroid (needs more testing)

### Limitations and known bugs
* Total size of the input file is limited such that the product of the dimensions should not exceed 1billion (should not be an issue for ships less than 1.5km in size)
* when exporting as a custom block type the required materials will state that you need standard grey blocks, the saved blocks are correct but the material list needs fixing
* As the program uses system commends to clear past files, the provided code only works on windows currently

### How to use
Ether use the pre compiled windows exe from [here](https://github.com/TBTerra/Vox2sm3/tree/master/bin) or download the source and compile with ```g++ Vox2sm3.cpp -lz -O2 -o Vox2sm3```

The first thing to set up is model its self. Position your model in your editor of choice sutch that positive Z is forwards and positive Y is up, and move the origin point to where you would like the core to be placed (as shown in the picture). then save the model in a format the binvox program can use.

![eve hulk 3d model positioned to export](TUT/obj.PNG?raw=true)

You then need to use [binvox](http://www.patrickmin.com/binvox/) to convert the model into voxels, the required conversion flag in ```-fit``` but ```-cb``` and ```-e``` are both highly recommended

Now you can use Vox2sm3 to generate a folder that can be dropped directly into the blueprints folder.

Useage is ```Vox2sm3 NAME_OF_MODEL``` do not add .binvox to the end of the name, as that is added automaticly when opening the file

If using the ```-b``` flag to use a different block you will need to provide both the block id and the block health, as the program does not have a list of block healths internally

![ingame image of hulk import](TUT/game.png?raw=true)