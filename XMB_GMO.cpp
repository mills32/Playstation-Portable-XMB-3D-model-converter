
//mesh: object vertices 
//Map: tga image
//fcurve-0 + 0x20: animate rotation (float values)
//fcurve-1 + 0x20: animate scale (float values)


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#define GU_ABGR(a,b,g,r)	(((a) << 24)|((b) << 16)|((g) << 8)|(r))

/*
GMO FILE
--------

1-CHUNKS:
	FILE {
		MODEL {
			BONE
			PART {
				MESH-0
				ARRAY-0
			}
			MATERIAL {
				LAYER
			}
			TEXTURE
			MOTION {
				FCURVES
			}
		}
	}

*/
typedef unsigned char u8; 
typedef signed char s8; 
typedef unsigned short u16; 
typedef unsigned long u32; 

//Everything must be 4 byte aligned 
typedef struct {
	float u,v;		//tex coords
	u32 rgba;		//vertex color
	float nx,ny,nz;	//vertex normal
	float x,y,z;	//vertex position
} GMO_Vertex;

unsigned char Default_Texture[172] = {
	0x00, 0x00, 0x0A, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x20, 0x00, 0x20, 0x00, 0x18, 0x00, 0x9F, 0xFF, 0xFF, 0xFF, 0x9F, 0xFF,
	0xFF, 0xFF, 0x9F, 0xFF, 0xFF, 0xFF, 0x9F, 0xFF, 0xFF, 0xFF, 0x9F, 0xFF,
	0xFF, 0xFF, 0x9F, 0xFF, 0xFF, 0xFF, 0x9F, 0xFF, 0xFF, 0xFF, 0x9F, 0xFF,
	0xFF, 0xFF, 0x9F, 0xFF, 0xFF, 0xFF, 0x9F, 0xFF, 0xFF, 0xFF, 0x9F, 0xFF,
	0xFF, 0xFF, 0x9F, 0xFF, 0xFF, 0xFF, 0x9F, 0xFF, 0xFF, 0xFF, 0x9F, 0xFF,
	0xFF, 0xFF, 0x9F, 0xFF, 0xFF, 0xFF, 0x9F, 0xFF, 0xFF, 0xFF, 0x9F, 0xFF,
	0xFF, 0xFF, 0x9F, 0xFF, 0xFF, 0xFF, 0x9F, 0xFF, 0xFF, 0xFF, 0x9F, 0xFF,
	0xFF, 0xFF, 0x9F, 0xFF, 0xFF, 0xFF, 0x9F, 0xFF, 0xFF, 0xFF, 0x9F, 0xFF,
	0xFF, 0xFF, 0x9F, 0xFF, 0xFF, 0xFF, 0x9F, 0xFF, 0xFF, 0xFF, 0x9F, 0xFF,
	0xFF, 0xFF, 0x9F, 0xFF, 0xFF, 0xFF, 0x9F, 0xFF, 0xFF, 0xFF, 0x9F, 0xFF,
	0xFF, 0xFF, 0x9F, 0xFF, 0xFF, 0xFF, 0x9F, 0xFF, 0xFF, 0xFF, 0x9F, 0xFF,
	0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x54, 0x52,
	0x55, 0x45, 0x56, 0x49, 0x53, 0x49, 0x4F, 0x4E, 0x2D, 0x58, 0x46, 0x49,
	0x4C, 0x45, 0x2E, 0x00
};


GMO_Vertex *ModelTempData;
GMO_Vertex *ModelData_Final;
u32 model_data_size = 0;
int normals = 0;

//GMO MODEL
typedef struct{
	u8 header[16] = {
		0x4F,0x4D,0x47,0x2E,0x30,0x30,0x2E,0x31,
		0x50,0x53,0x50,0x00,0x00,0x00,0x00,0x00
	};
	//FILE CHUNK HEADER
	u16 file_entry = 0x0002;
	u16 file_header_size = 0x0010; 
	u32 file_chunk_size = 0x00000010;	//model_chunk_size+16
	u32 file_model_offset = 0x00000010;	//chunk MODEL
	u32 file_chunk_data = 0x00000010;	//MODEL is the data
	
	//MODEL CHUNK HEADER
	u16 model_entry = 0x0003;
	u16 model_header_size = 0x0014; 
	u32 model_chunk_size = 0x00000014;//bone+part+material+texture
	u32 model_bone_offset = 0x00000014;//Total size to reach "BOUNDING BOX"
	u32 model_chunk_data = 0x00000014;
	u32 model_chunk_name = 0x5F4C444D;	//"MDL_" aligned 4byte
	
	//BONE CHUNK HEADER
	u16 bone_entry = 0x0004;
	u16 bone_header_size = 0x0014; 
	u32 bone_chunk_size = 0x00000014+12;	//total size to reach "PART" offset ;
	u32 bone_command_offset = 0x00000014;	//bone commands
	u32 bone_chunk_data = 0x00000014;	//bone commands
	u32 bone_name = 0x454E4F42;//"BONE"
	//BONE CHUNK DATA DRAW PART: REFERENCE 1 higher levels exists, first chunk "PART" (10); type 5 (05)
	u32 bone_draw_part[3] = {0x0000804E,0x0000000C,0x00051000};
	
	//PART HEADER
	u16 part_entry = 0x0005;
	u16 part_header_size = 0x0014; 
	u32 part_chunk_size = 0x00000014;	//total size (mesh + array) to reach "MATERIAL" offset 
	u32 part_mesh_offset = 0x00000014;	//chunk MESH
	u32 part_chunk_data = 0x00000014;	//MESH is the data
	u32 part_name = 0x54524150;//"PART"
	
	//MESH HEADER
	u16 mesh_entry = 0x0006;
	u16 mesh_header_size = 0x0014; 
	u32 mesh_chunk_size = 0x00000038;	//total size (header+data) to reach "ARRAY" offset;
	u32 mesh_command_offset = 0x00000014;
	u32 mesh_chunk_data = 0x00000014;
	u32 mesh_name = 0x4853454D;//"MESH"
	//MESH DATA
	u32 mesh_set_material[3] = {0x00008061,0x0000000C,0x00081000};//REF MATERIAL 10 08
	u32 mesh_draw_array[6] = {0x00008066,0x00000018,0x00072000,3,3,0};//REF ARRAY 20 07,3,3,n tris

	//ARRAY HEADER
	u16 array_entry = 0x0007;
	u16 array_header_size = 0x0014; 
	u32 array_chunk_size = 0x00000014 + 16;//total size (header + info + data) to "MATERIAL" offset
	u32 array_next_chunk_offset = 0x00000014 + 16;//total size (header + info + data) to "MATERIAL" offset
	u32 array_chunk_data = 0x00000014 + 16;
	u32 array_name = 0x54524556;//"VERT"
	//ARRAY INFO
	//format 0: 01FF (T float C RGBA N byte V float), 
	// N verts, 1 morph, 0
	u32 array_settings[4] = {0x000001FF,0,1,0};
	//ARRAY DATA
	GMO_Vertex *array_data;
	
	//MATERIAL HEADER
	u16 material_entry = 0x0008;
	u16 material_header_size = 0x0014; 
	u32 material_chunk_size = 0x00000088;//Total size (header + data + layer) to "TEXTURE" offset
	u32 material_command_offset = 0x00000014; //
	u32 material_chunk_data = 0x00000014; //
	u32 material_name = 0x3038202D;
	//MATERIAL DATA
	u32 material_diffuse[2] = {0x00008082,0x00000018};
	float material_diffuse_color[4] = {1,1,1,1};//TEXTCOORD = 1,1,1,1; ENVMAP = 0,0,0,0
	u32 material_reflection[2] = {0x00008086,0x0000000C};
	float material_reflection_factor = 0;//TEXTCOORD = 0;ENVMAP = 1;
	
	//LAYER HEADER
	u16 layer_entry = 0x0009;
	u16 layer_header_size = 0x0014; 
	u32 layer_chunk_size = 0x00000050;//total size (header + data) to "TEXTURE" offset
	u32 layer_command_offset = 0x00000014; //
	u32 layer_chunk_data = 0x00000014; //
	u32 layer_name = 0x6C617965;
	//LAYER DATA
	u32 layer_texture_set[4]     = {0x00008091,0x00000010,0x000A2000,0};//REFERENCE 20 0A	
	u32 layer_texture_mapping[4] = {0x00008092,0x0000000C,0x82};//0x82 TEXCOORD; 0x86 ENVMAP 
	u32 layer_texture_filter[4]  = {0x00008096,0x00000010,0,0}; //FILTER MAG, FILTER MIN (NOT WORKING)
	u32 layer_texture_wrap[4]    = {0x00008097,0x00000010,1,1}; //WRAP U, WRAP V (NOT WORKING)
	
	//TEXTURE HEADER
	u16 texture_entry = 0x000A;
	u16 texture_header_size = 0x0014; 
	u32 texture_chunk_size = 0x00000014+12;//Total size (header + data/tga) to "MOTION" offset
	u32 texture_command_offset = 0x00000014; //
	u32 texture_chunk_data = 0x00000014; //
	u32 texture_name = 0x5F474D49;//"IMG_" 
	//TEXTURE DATA (TGA file included)
	u32 texture_info[3] = {0x00008013,0,0};//SIZE OF TGA CHUNK, SIZE OF TGA DATA
	u8 *texture_TGA;//TGA file
	
	//ANIMATION HEADER
	u16 anim_entry = 0x000B;
	u16 anim_header_size = 0x0014; 
	u32 anim_chunk_size = 0x00000230;//total size (anim + fcurves) to end of file
	u32 anim_command_offset = 0x00000014; //
	u32 anim_chunk_data = 0x00000014; //
	u32 anim_name = 0x4D494E41; //"ANIM" 
	//ANIMATION DATA (+FCURVES)
	u32 anim_set_frameloop[2] = {0x000080B1,0x00000010};
	float anim_frameloop[2] = {0,240};
	u32 anim_set_framerate[2] = {0x000080B2,0x0000000C};
	float anim_framerate = 10;
	//ANIMATIONS             SETANIM    SIZE       REF CHUNK  ANIM TYPE  MODE FCURVE
	u32 anim_translate[6] = {0x000080B3,0x00000018,0x00041000,0x00000048,  0, 0x000C0000};
	u32 anim_scale[6]     = {0x000080B3,0x00000018,0x00041000,0x0000004C,  0, 0x000C0001};
	u32 anim_rotate[6]    = {0x000080B3,0x00000018,0x00041000,0x0000004A,  0, 0x000C0002};
	u32 anim_UVWH[6]      = {0x000080B3,0x00000018,0x00081000,0x00000098,  0, 0x000C0003};
	
	//FCURVE0 (TRASLATE) HEADER
	u16 fcurve0_entry = 0x000C;
	u16 fcurve0_header_size = 0x0014; 
	u32 fcurve0_chunk_size = 0x00000064;//size of fcurve data + header
	u32 fcurve0_next_curve = 0x00000064;//size of fcurve data + header
	u32 fcurve0_data_offset = 0x00000024;
	u32 fcurve0_name = 0x30525543;//"CUR0" 
	//SETTINGS: INTERPOLATION FORMAT, DIMENSIONS, KEY FRAMES, 0
	u32 fcurve0_settings[4] = {1, 3, 4, 0};
	//FCURVE DATA: FRAME NUMER, VALUE(s)
	float fcurve0_data[4*4] = {0,0,0,0, 80,0,0,0, 160,0,0,0, 240,0,0,0};
	
	//FCURVE1 (SCALE) HEADER
	u16 fcurve1_entry = 0x000C;
	u16 fcurve1_header_size = 0x0014; 
	u32 fcurve1_chunk_size = 0x00000064;//size of fcurve data + header
	u32 fcurve1_next_curve = 0x00000064;//size of fcurve data + header
	u32 fcurve1_data_offset = 0x00000024;
	u32 fcurve1_name = 0x31525543;//"CUR1" 
	//SETTINGS: INTERPOLATION FORMAT, DIMENSIONS, KEY FRAMES, 0
	u32 fcurve1_settings[4] = {1, 3, 4, 0};
	//FCURVE DATA: FRAME NUMER, VALUE(s)
	float fcurve1_data[4*4] = {0,1,1,1, 80,1,1,1, 160,1,1,1, 240,1,1,1};
	
	//FCURVE2 (ROTATE) HEADER
	u16 fcurve2_entry = 0x000C;
	u16 fcurve2_header_size = 0x0014; 
	u32 fcurve2_chunk_size = 0x00000064;//size of fcurve data + header
	u32 fcurve2_next_curve = 0x00000064;//size of fcurve data + header
	u32 fcurve2_data_offset = 0x00000024;
	u32 fcurve2_name = 0x32525543;//"CUR2" 
	//SETTINGS: INTERPOLATION FORMAT, DIMENSIONS, KEY FRAMES, 0
	u32 fcurve2_settings[4] = {1, 3, 4, 0};
	//FCURVE DATA: FRAME NUMER, VALUE(s) ///6.28319
	float fcurve2_data[4*4] = {0,0,0,0, 80,0,0,0, 160,0,0,0, 240,0,0,0};
	
	//FCURVE3 (UV) HEADER
	u16 fcurve3_entry = 0x000C;
	u16 fcurve3_header_size = 0x0014; 
	u32 fcurve3_chunk_size = 0x00000074;//size of fcurve data + header
	u32 fcurve3_next_curve = 0x00000074;//size of fcurve data + header
	u32 fcurve3_data_offset = 0x00000024;
	u32 fcurve3_name = 0x33525543;//"CUR3" 
	u32 fcurve3_settings[4] = {1, 4, 4, 0};
	//FCURVE DATA: FRAME NUMER, VALUE(s)
	float fcurve3_data[4*5] = {0,0,0,1,1, 80,0,0,1,1, 160,0,0,1,1, 240,0,0,1,1};
}GMO_OBJ;

GMO_OBJ object;

int LoadModelPLY(FILE *f, int enable_normals){
	ModelTempData = NULL;
	ModelData_Final = NULL;
	if(!f) return 0;
	u32 *vindex; 
	u32 vtx = 0;
	u32 nvindex = 0;
	u32 nvtx = 0;
	u32 n_faces = 0;
	u8 vertex_xyz  = 0;
	u8 vertex_n = 0;
	u8 vertex_st = 0;
	u8 vertex_rgb  = 0;
	u32 i = 0;
	char *line = (char*) calloc (128, sizeof(char));
	int colorR,colorG,colorB = 0;
	nvtx = 0;
	nvindex = 0;
	//READ PARAMETERS
	fseek(f, 0, SEEK_SET);
	while(!feof(f)){
		memset(line, 0, 128);
		fgets(line, 128, f);
		if (line[0] == 'e'){
			if(line[8] == 'v'){			
				sscanf(line, "element vertex %lu",&nvtx);
			}
			if(line[8] == 'f'){			
				sscanf(line, "element face %lu",&n_faces);
				break;
			}			
		}
		if (line[0] == 'p'){
			if(line[15] == 'z') vertex_xyz = 1;
			if(line[16] == 'z') vertex_n = 1;
			if(line[15] == 't') vertex_st = 1;
			if(line[15] == 'b') vertex_rgb = 1;
		}
	}
	
	if (!vertex_xyz) return 0;//Error
	if (!vertex_n) return 0;//Error
	if (!vertex_st) return 0;//Error

	//STORE VERTICES HERE
	ModelTempData = (GMO_Vertex *) calloc(nvtx,sizeof(GMO_Vertex));
	ModelData_Final = (GMO_Vertex *) calloc(n_faces*3,sizeof(GMO_Vertex));
	model_data_size = n_faces*3*sizeof(GMO_Vertex);
	
	//READ VERTICES
	fseek(f, 0, SEEK_SET);
	while(!feof(f)){
		memset(line, 0, 128);
		fgets(line, 128, f);
		if ((line[0] < 58)&&(line[1] != ' ')){//ascii, not a letter, and not a space in 1.
			//x y z nx ny nz u v color
			sscanf(line,"%f %f %f %f %f %f %f %f %d %d %d ",
				&ModelTempData[vtx].x ,&ModelTempData[vtx].y ,&ModelTempData[vtx].z,
				&ModelTempData[vtx].nx,&ModelTempData[vtx].ny,&ModelTempData[vtx].nz,	
				&ModelTempData[vtx].u ,&ModelTempData[vtx].v,
				&colorR, &colorG, &colorB);
			if (vertex_rgb) ModelTempData[vtx].rgba = GU_ABGR(255,colorB,colorG,colorR);
			else ModelTempData[vtx].rgba = 0xFFFFFFFF;
			if (!enable_normals){
				ModelTempData[vtx].nx = 0;
				ModelTempData[vtx].ny = 0;
				ModelTempData[vtx].nz = 1;
			}
			//next vertex
			//invert texture v coords
			ModelTempData[vtx].v = (-1*ModelTempData[vtx].v)+1;
			vtx++;
			if (vtx == nvtx) break;
		}
	}
	vtx = 0;
	rewind(f);
	
	//INDICES 
	vindex = (u32 *) calloc(n_faces*3,sizeof(u32));
	fseek(f, 0, SEEK_SET);
	while(!feof(f)){
		memset(line, 0, 128);
		fgets(line, 128, f);
		if ((line[0] == '3')&&(line[1] == ' ')){//ascii, equal 3 (avoid 3.00...)
			sscanf(line,"3 %ld %ld %ld",&vindex[nvindex],&vindex[nvindex+1],&vindex[nvindex+2]);
			nvindex+=3;
			vtx++;
			if(vtx == n_faces) break;
		}
	}
	fclose(f);
	
	//ARRANGE VERTICES
	for (i = 0; i < n_faces*3;i++){
		u32 index = vindex[i];
		memcpy(&ModelData_Final[i],&ModelTempData[index],sizeof(GMO_Vertex));
	}
	
	//Store in GMO model
	object.array_data = (GMO_Vertex*) calloc(n_faces*3,sizeof(GMO_Vertex));
	memcpy(object.array_data,ModelData_Final,model_data_size);
	object.array_chunk_size += model_data_size;
	object.array_settings[1] = n_faces*3;
	object.mesh_draw_array[5] = n_faces;
	printf("LOADED VERTICES %08X; TRIS %08X \n",n_faces*3,n_faces);
	printf("array_chunk_size %08X\n",object.array_chunk_size);

	//Free used ram
	if(vindex) {free(vindex); vindex = NULL;     }
	if(line) {free(line); line = NULL;     }
	if(ModelTempData) {free(ModelTempData); ModelTempData = NULL;  }
	if(ModelData_Final) {free(ModelData_Final); ModelData_Final = NULL;  }

	return 1;
}

u32 Read_TGA(FILE *f){
	u32 image_file_size = 0;
	if(f){
		fseek(f,0,SEEK_END);
		image_file_size = ftell(f);
		image_file_size += (4-(image_file_size&3));//align 4 byte
		fseek(f,0,SEEK_SET);
		object.texture_info[1] = image_file_size+12;//TGA chunk header
		object.texture_info[2] = image_file_size;
		object.texture_TGA = (u8*) calloc(image_file_size,1);
		fread(object.texture_TGA,1,image_file_size,f);
		fclose(f);
		object.texture_chunk_size += image_file_size;
		printf("LOADED TGA %08X\n",image_file_size);
		printf("texture_chunk_size %08X\n\n",object.texture_chunk_size);
		return image_file_size;
	} else {
		image_file_size = 172;
		object.texture_info[1] = image_file_size+12;//TGA chunk header
		object.texture_info[2] = image_file_size;
		object.texture_TGA = (u8*) Default_Texture;
		object.texture_chunk_size += image_file_size;
		return image_file_size;
	}
}

int Read_TXT(FILE *txt){
	if (txt){
		printf("\nREADING SETTINGS TXT\n");
		char *line = (char*) calloc (256, sizeof(char));
		
		//READ PARAMETERS
		fseek(txt, 0, SEEK_SET);
		fgets(line, 256, txt);
		if ( line[0] != 'X' && line[1] != 'M' &&  line[2] != 'B'  
			&& line[4] != 'G' && line[5] != 'M' && line[6] != 'O' ) return 0;
		fgets(line, 256, txt);
		fgets(line, 256, txt);
		
		//GET FILTER (I could not find the values needed to set "NEAREST" filter)
		fgets(line, 256, txt);
		float filter;
		sscanf(line, "F %f",&filter);
		if (filter == 0.000000) {
			object.layer_texture_filter[3] = 0;
			object.layer_texture_filter[4] = 0;
			printf("	TEXTURE FILTER OFF\n");
		}
		if (filter == 1.000000) {
			object.layer_texture_filter[3] = 1;
			object.layer_texture_filter[4] = 1;
			printf("	TEXTURE FILTER ON\n");
		}
		fgets(line, 256, txt);
		
		//GET MAPPING
		fgets(line, 256, txt);
		float mapping;
		sscanf(line, "T %f",&mapping);
		if (mapping == 0.000000) {
			object.material_diffuse_color[0] = 1;
			object.material_diffuse_color[1] = 1;
			object.material_diffuse_color[2] = 1;
			object.material_diffuse_color[3] = 1;
			object.material_reflection_factor = 0;
			object.layer_texture_mapping[2] = 0x82;
			normals = 0;
			printf("	TEXTURE MAPPING: UV COORDS\n");
		}
		if (mapping == 1.000000) {
			object.material_diffuse_color[0] = 0;
			object.material_diffuse_color[1] = 0;
			object.material_diffuse_color[2] = 0;
			object.material_diffuse_color[3] = 0;
			object.material_reflection_factor = 1;
			object.layer_texture_mapping[2] = 0x86;
			normals = 1;
			printf("	TEXTURE MAPPING: ENV MAPPING\n");
		}

		fgets(line, 256, txt);
		//GET FPS
		fgets(line, 256, txt);	
		sscanf(line, "S %f",&object.anim_framerate);
		printf("	ANIMATION SPEED %f FPS\n",object.anim_framerate);
		fgets(line, 256, txt);
		
		//GET TRANSLATE ANIMATION
		fgets(line, 256, txt);
		sscanf(line, "1 %f,%f,%f",&object.fcurve0_data[1],&object.fcurve0_data[2],&object.fcurve0_data[3]);
		fgets(line, 256, txt);
		sscanf(line, "2 %f,%f,%f",&object.fcurve0_data[5],&object.fcurve0_data[6],&object.fcurve0_data[7]);
		fgets(line, 256, txt);
		sscanf(line, "3 %f,%f,%f",&object.fcurve0_data[9],&object.fcurve0_data[10],&object.fcurve0_data[11]);
		fgets(line, 256, txt);
		sscanf(line, "4 %f,%f,%f",&object.fcurve0_data[13],&object.fcurve0_data[14],&object.fcurve0_data[15]);
		fgets(line, 256, txt);
		
		//GET SCALE ANIMATION
		fgets(line, 256, txt);
		sscanf(line, "1 %f,%f,%f",&object.fcurve1_data[1],&object.fcurve1_data[2],&object.fcurve1_data[3]);
		fgets(line, 256, txt);
		sscanf(line, "2 %f,%f,%f",&object.fcurve1_data[5],&object.fcurve1_data[6],&object.fcurve1_data[7]);
		fgets(line, 256, txt);
		sscanf(line, "3 %f,%f,%f",&object.fcurve1_data[9],&object.fcurve1_data[10],&object.fcurve1_data[11]);
		fgets(line, 256, txt);
		sscanf(line, "4 %f,%f,%f",&object.fcurve1_data[13],&object.fcurve1_data[14],&object.fcurve1_data[15]);
		fgets(line, 256, txt);

		//GET ROTATE ANIMATION
		fgets(line, 256, txt);
		sscanf(line, "1 %f,%f,%f",&object.fcurve2_data[1],&object.fcurve2_data[2],&object.fcurve2_data[3]);
		fgets(line, 256, txt);
		sscanf(line, "2 %f,%f,%f",&object.fcurve2_data[5],&object.fcurve2_data[6],&object.fcurve2_data[7]);
		fgets(line, 256, txt);
		sscanf(line, "3 %f,%f,%f",&object.fcurve2_data[9],&object.fcurve2_data[10],&object.fcurve2_data[11]);
		fgets(line, 256, txt);
		sscanf(line, "4 %f,%f,%f",&object.fcurve2_data[13],&object.fcurve2_data[14],&object.fcurve2_data[15]);
		fgets(line, 256, txt);

		//GET TEXTURE ANIMATION
		fgets(line, 256, txt);
		sscanf(line, "1 %f,%f,%f,%f",&object.fcurve3_data[1],&object.fcurve3_data[2],&object.fcurve3_data[3],&object.fcurve3_data[4]);
		fgets(line, 256, txt);
		sscanf(line, "2 %f,%f,%f,%f",&object.fcurve3_data[6],&object.fcurve3_data[7],&object.fcurve3_data[8],&object.fcurve3_data[9]);
		fgets(line, 256, txt);
		sscanf(line, "3 %f,%f,%f,%f",&object.fcurve3_data[11],&object.fcurve3_data[12],&object.fcurve3_data[13],&object.fcurve3_data[14]);
		fgets(line, 256, txt);
		sscanf(line, "4 %f,%f,%f,%f",&object.fcurve3_data[16],&object.fcurve3_data[17],&object.fcurve3_data[18],&object.fcurve3_data[19]);
		return 1;
	} else return 0;
}

//---------------------------------------------------------------------------------
int main(int argc, char **argv) {
//---------------------------------------------------------------------------------
	FILE *tga;
	FILE *ply;
	FILE *txt;
	FILE *gmo;
	u32 image_file_size = 0;
	size_t filelen;
	int linelen;

	if(argc < 2) {
		printf(	"\n\n\n\n"
				"EDIT PSP XMB WAVES (GMO 3D MODEL)\n"
				"---------------------------------\n"
				"usage: XMB_GMO name.tga (or name.ply, or name.txt)"
				"\n\n\n\n");
		return 0;
	}
	printf("\n\n\n\n");
	tga = fopen(argv[1], "rb");
	if(!tga) {
		printf("XMB_GMO: could not open TGA image file\n");
		printf("         Model will not use textures\n\n");
  	}
	ply = fopen(argv[2], "rb");
	if(!ply) {
		printf("XMB_GMO: could not open PLY model file\n\n");
		perror(argv[2]);
		return 1;
  	}
	txt = fopen(argv[3], "rb");
	if(!txt) {
		printf("XMB_GMO: could not open TXT animation file\n");
		printf("         Model will not be animated\n\n");
  	}
  	gmo = fopen(argv[4], "wb");
  	if(!gmo) {
		printf("XMB_GMO: could not create GMO model file\n\n");
		perror(argv[4]);
		return 1;
  	}
	
	//READ TXT
	Read_TXT(txt);
	//LOAD PLY
	LoadModelPLY(ply,normals);
	//READ TGA
	image_file_size = Read_TGA(tga);
	//SET SIZES
	object.part_chunk_size += (object.mesh_chunk_size+object.array_chunk_size);
	object.model_chunk_size +=  ( object.bone_chunk_size
								+ object.part_chunk_size
								+ object.material_chunk_size
								+ object.texture_chunk_size
								+ object.anim_chunk_size);
	object.file_chunk_size = object.model_chunk_size + 16;
	
	//WRITE GMO
	fwrite(object.header,1,16,gmo);
	fwrite(&object.file_entry,1,object.file_header_size,gmo);
	fwrite(&object.model_entry,1,object.model_header_size,gmo);
	fwrite(&object.bone_entry,1,object.bone_chunk_size,gmo);
	fwrite(&object.part_entry,1,object.part_header_size,gmo);
	fwrite(&object.mesh_entry,1,object.mesh_chunk_size,gmo);
	fwrite(&object.array_entry,1,object.array_header_size,gmo);
	fwrite(object.array_settings,1,16,gmo);
	fwrite(object.array_data,1,model_data_size,gmo);
	fwrite(&object.material_entry,1,object.material_chunk_size,gmo);
	fwrite(&object.texture_entry,1,object.texture_header_size,gmo);
	fwrite(object.texture_info,1,12,gmo);
	fwrite(object.texture_TGA,1,image_file_size,gmo);
	fwrite(&object.anim_entry,1,object.anim_chunk_size,gmo);
	
	
	free(object.texture_TGA);
	fclose(ply);
	//fclose(txt);
	fclose(gmo);

	return 0;
}


