/*
	Tower3D v1.4

	Hakkında
	- Bunu yapmaktaki amacım öncelikle kendim için oyun geliştirmeyi kolaylaştırmaktı.
	- Fakat sonradan şunu fark ettim, her seferinde sıfırdan yazmak gereksiz iş yükü ve
	zaman kaybıydı. İşleri biraz hızlandırmak lazım.
	- Tabi hızlandırırken kontrolü kesinlikle elden bırakmamalıyız.
	- Bu nedenle yavaş yavaş geliştireceğim bu dosyayı kendim dahil herkesin denemesi
	için yazdım. Tek seferde en iyisini yapamayacağımın farkındayım.
	- İsterseniz benim dağıttığım dll dosyalarını kullanın, isterseniz kendiniz implemente
	edin. Bunlar size kalmış.
	- Yani temeldeki amacım olabildiğince az GPU call kullanarak çalışmak.
	- Bunun dışında umarım işinize yarar.
	- Herkese iyi çalışmalar diliyorum.

	Q&A
	- Neden OpenGL 3.3 veya 4.5 değil de 1.1 kullanıyorsun?
		- Çünkü hangi sürümünü kullanırsan kullan state machine ile sınırlanıyorsun zaten
		- Ben de açıkçası bu yüzden 1.1'i biraz cımbızlayarak oluşturdum bunu.
		- Sadece işime yarayabilecek bazı özellikleri nispeten yeni sürümlerden kullanıyorum.
		- Gerçi bu sürümden (tw3d.h v1.4) itibaren oldukça karma bir OpenGL kullanımı var.
	- Neden sürekli instance gönderiyoruz?
		- Başka mimari tercihlere uygun olması adına böyle düşündüm.
	- THREAD_SAFE niye?
		- Çünkü OpenGL değil, fakat dışındakiler için kullanışlı olacağını düşündüm.
		- Zira Thread için de bir şeyler düşüneceğim.
	- GPU Call sayısı niye?
		- En azından kendi implementasyonumda CPU ve GPU kullanımının farkını görmeniz için.
		- OpenGL 1.1 ve 4.6 arasındaki işlevlerden karma bir yapı kuracağımdan kendiniz deneyin.

	TL;DR Plan:
	- Aklımda yukarda uzata uzata anlattığım şey aslında bir şartname oluşturmak.
	- Matematik + Pencere + Grafik üçlüsünün birleşimi hedefim olacak.
	- İsteyen herkes kendi uygulamasını yapar bu dosya üzerinden. Emin olun oldukça öğreticidir.
	- Shader yazmak için ARB asm dilini kullanıyorum. Emin olun GLSL'den daha iyi bir seçenek.

	Hastasına Bilgi:
	- Çalışırken King's Field 2 - West Shore dinliyorum, size de öneriririm.

	Sistemin Çalışma Mantığı:
	- Tahmin edebileceğiniz üzere bütün başlatma ayarları TwInstance içinde bulunuyor olacak.
	- TwInstance'ın düzgün başlatılabilmesi için TwInstanceDesc ile yaratmamız gerekiyor.
	- Index ile texture bind yapmak isterseniz önce TwInstanceAllocator ile alan ayırmalısınız.
	- Bunların dışında ekstra bir adım yok gerisi size kalmış.

	TODO:
	- Vertexleri toplu yollayacak bir sistem iyi olur ve tabi ARB için de
	- VRAM'e buffer gönderme işini basitleştireceğim.
	- Pencere işlevleri artmalı
	- Debug layer???
	- VRAM parselleme & FBO
	- font & text fonksiyonları işe yarar gibi
	- model format + spirv (spirv-as) + refactor + input
	- tw3d_audio.h

	Kaynaklar:
	- https://registry.khronos.org/OpenGL/api/GL/glext.h
	- https://registry.khronos.org/OpenGL/extensions/ARB/ARB_vertex_program.txt
	- https://registry.khronos.org/OpenGL/extensions/ARB/ARB_fragment_program.txt
	- https://community.khronos.org/t/doom-3/37313/3
	- https://www.lunarg.com/wp-content/uploads/2023/05/SPIRV-Osaka-MAY2023.pdf
	- https://registry.khronos.org/SPIR-V/specs/unified1/SPIRV.html
	- https://github.com/KhronosGroup/SPIRV-Headers/blob/main/include/spirv/1.0/spirv.h // SpvOp* ile ara

	Güzel İnsanlara Teşekkürler:
	- Sean Barrett
*/

typedef int TwInt;
typedef int TwSizei;
typedef float TwFloat;
typedef double TwDouble;
typedef long long TwSizeiptr;
typedef unsigned int TwFlag;
typedef unsigned int TwUint;
typedef unsigned int TwID;
typedef unsigned char TwByte;
typedef unsigned int TwBoolean;
#define TW_TRUE (TwBoolean)1
#define TW_FALSE (TwBoolean)0
typedef struct { TwFloat m[16]; } TwMat4;
typedef struct { TwFloat x, y, z; } TwVec3;
typedef struct {
	const TwFloat* data;
	TwUint vertex_count;
	TwUint mode;
} TwT3D;

typedef struct TwInstance TwInstance;
typedef struct TwInstanceDesc {
	TwUint width, height, flag;
	TwBoolean culling;
	const char* title;
	TwByte exit_key;
} TwInstanceDesc;

typedef enum {
	TW_VERTEX_PROGRAM_ARB           = 0x8620,
	TW_FRAGMENT_PROGRAM_ARB         = 0x8804,

	TW_PROGRAM_FORMAT_ASCII_ARB     = 0x8875,
	TW_MAX_PROGRAM_INSTRUCTIONS_ARB = 0x88A1,
	TW_MAX_PROGRAM_TEMPORARIES_ARB  = 0x88A5,

	TW_TEXTURE0_ARB              = 0x84C0,
	TW_TEXTURE1_ARB              = 0x84C1,
	TW_TEXTURE2_ARB              = 0x84C2,
	TW_TEXTURE3_ARB              = 0x84C3,
	TW_TEXTURE4_ARB              = 0x84C4,
	TW_TEXTURE5_ARB              = 0x84C5,
	TW_TEXTURE6_ARB              = 0x84C6,
	TW_TEXTURE7_ARB              = 0x84C7,
	TW_TEXTURE8_ARB              = 0x84C8,
	TW_TEXTURE9_ARB              = 0x84C9,
	TW_TEXTURE10_ARB             = 0x84CA,
	TW_TEXTURE11_ARB             = 0x84CB,
	TW_TEXTURE12_ARB             = 0x84CC,
	TW_TEXTURE13_ARB             = 0x84CD,
	TW_TEXTURE14_ARB             = 0x84CE,
	TW_TEXTURE15_ARB             = 0x84CF,
	TW_TEXTURE16_ARB             = 0x84D0,
	TW_TEXTURE17_ARB             = 0x84D1,
	TW_TEXTURE18_ARB             = 0x84D2,
	TW_TEXTURE19_ARB             = 0x84D3,
	TW_TEXTURE20_ARB             = 0x84D4,
	TW_TEXTURE21_ARB             = 0x84D5,
	TW_TEXTURE22_ARB             = 0x84D6,
	TW_TEXTURE23_ARB             = 0x84D7,
	TW_TEXTURE24_ARB             = 0x84D8,
	TW_TEXTURE25_ARB             = 0x84D9,
	TW_TEXTURE26_ARB             = 0x84DA,
	TW_TEXTURE27_ARB             = 0x84DB,
	TW_TEXTURE28_ARB             = 0x84DC,
	TW_TEXTURE29_ARB             = 0x84DD,
	TW_TEXTURE30_ARB             = 0x84DE,
	TW_TEXTURE31_ARB             = 0x84DF,

	TW_ACTIVE_TEXTURE_ARB        = 0x84E0,
	TW_CLIENT_ACTIVE_TEXTURE_ARB = 0x84E1,
	TW_MAX_TEXTURE_UNITS_ARB     = 0x84E2,
} TwARB;

typedef enum {
	TW_FEATURE_TEXTURE_2D = 1 << 0,
	TW_FEATURE_LIGHTING = 1 << 1,
	TW_FEATURE_BLEND = 1 << 2,
	TW_FEATURE_FOG = 1 << 3,
	TW_FEATURE_NORMALIZE = 1 << 4,
	TW_FEATURE_DEPTH_TEST = 1 << 5,
	TW_FEATURE_ALPHA_TEST = 1 << 6,
	TW_FEATURE_COLOR_MATERIAL = 1 << 7,
	TW_FEATURE_CULL_FACE = 1 << 8,

	TW_FEATURE_LIGHT0 = 1 << 9,
	TW_FEATURE_LIGHT1 = 1 << 10,
	TW_FEATURE_LIGHT2 = 1 << 11,
	TW_FEATURE_LIGHT3 = 1 << 12,
	TW_FEATURE_LIGHT4 = 1 << 13,
	TW_FEATURE_LIGHT5 = 1 << 14,
	TW_FEATURE_LIGHT6 = 1 << 15,
	TW_FEATURE_LIGHT7 = 1 << 16,

	TW_FEATURE_LIGHTS = TW_FEATURE_LIGHT0 | TW_FEATURE_LIGHT1 | TW_FEATURE_LIGHT2 | TW_FEATURE_LIGHT3 | TW_FEATURE_LIGHT4 | TW_FEATURE_LIGHT5 | TW_FEATURE_LIGHT6 | TW_FEATURE_LIGHT7,
	TW_FEATURE_FOG_AND_LIGHTING = TW_FEATURE_FOG | TW_FEATURE_LIGHTING,
} TwFeature;

typedef enum {
	TW_NONE = 0,
	// DescFlag
	TW_DESC_FLAG_FULLSCREEN,

	// TwID
	TW_ID_ERROR = 0xFFFF,

	// InstanceAlloc
	TW_ALLOC_POOL_TEX    = 0x0A,
	TW_ALLOC_POOL_SHADER = 0x0B,
	TW_ALLOC_POOL_MODEL  = 0x0C,

	// AttribMask
	TW_DEPTH_BUFFER_BIT   = 0x00000100,
	TW_STENCIL_BUFFER_BIT = 0x00000400,
	TW_COLOR_BUFFER_BIT   = 0x00004000,

	// BlendingFactorDest
	TW_ZERO                = 0,
	TW_ONE                 = 1,
	TW_SRC_COLOR           = 0x0300,
	TW_ONE_MINUS_SRC_COLOR = 0x0301,
	TW_SRC_ALPHA           = 0x0302,
	TW_ONE_MINUS_SRC_ALPHA = 0x0303,
	TW_DST_ALPHA           = 0x0304,
	TW_ONE_MINUS_DST_ALPHA = 0x0305,

	// BlendingFactorSrc
	TW_DST_COLOR           = 0x0306,
	TW_ONE_MINUS_DST_COLOR = 0x0307,
	TW_SRC_ALPHA_SATURATE  = 0x0308,

	// FogMode
	TW_EXP  = 0x0800,
	TW_EXP2 = 0x0801,

	// FrontFaceDirection
	TW_CW  = 0x0900,
	TW_CCW = 0x0901,

	// DrawBufferMode
	//TW_NONE           = 0,
	TW_FRONT_LEFT     = 0x0400,
	TW_FRONT_RIGHT    = 0x0401,
	TW_BACK_LEFT      = 0x0402,
	TW_BACK_RIGHT     = 0x0403,
	TW_FRONT          = 0x0404,
	TW_BACK           = 0x0405,
	TW_LEFT           = 0x0406,
	TW_RIGHT          = 0x0407,
	TW_FRONT_AND_BACK = 0x0408,
	TW_AUX0           = 0x0409,
	TW_AUX1           = 0x040A,
	TW_AUX2           = 0x040B,
	TW_AUX3           = 0x040C,

	// BeginMode
	TW_POINTS         = 0x0000,
	TW_LINES          = 0x0001,
	TW_LINE_LOOP      = 0x0002,
	TW_LINE_STRIP     = 0x0003,
	TW_TRIANGLES      = 0x0004,
	TW_TRIANGLE_STRIP = 0x0005,
	TW_TRIANGLE_FAN   = 0x0006,
	TW_QUADS          = 0x0007,
	TW_QUAD_STRIP     = 0x0008,
	TW_POLYGON        = 0x0009,

	// GetTarget
	TW_CURRENT_COLOR                  = 0x0B00,
	TW_CURRENT_INDEX                  = 0x0B01,
	TW_CURRENT_NORMAL                 = 0x0B02,
	TW_CURRENT_TEXTURE_COORDS         = 0x0B03,
	TW_CURRENT_RASTER_COLOR           = 0x0B04,
	TW_CURRENT_RASTER_INDEX           = 0x0B05,
	TW_CURRENT_RASTER_TEXTURE_COORDS  = 0x0B06,
	TW_CURRENT_RASTER_POSITION        = 0x0B07,
	TW_CURRENT_RASTER_POSITION_VALID  = 0x0B08,
	TW_CURRENT_RASTER_DISTANCE        = 0x0B09,
	TW_POINT_SMOOTH                   = 0x0B10,
	TW_POINT_SIZE                     = 0x0B11,
	TW_POINT_SIZE_RANGE               = 0x0B12,
	TW_POINT_SIZE_GRANULARITY         = 0x0B13,
	TW_LINE_SMOOTH                    = 0x0B20,
	TW_LINE_WIDTH                     = 0x0B21,
	TW_LINE_WIDTH_RANGE               = 0x0B22,
	TW_LINE_WIDTH_GRANULARITY         = 0x0B23,
	TW_LINE_STIPPLE                   = 0x0B24,
	TW_LINE_STIPPLE_PATTERN           = 0x0B25,
	TW_LINE_STIPPLE_REPEAT            = 0x0B26,
	TW_LIST_MODE                      = 0x0B30,
	TW_MAX_LIST_NESTING               = 0x0B31,
	TW_LIST_BASE                      = 0x0B32,
	TW_LIST_INDEX                     = 0x0B33,
	TW_POLYGON_MODE                   = 0x0B40,
	TW_POLYGON_SMOOTH                 = 0x0B41,
	TW_POLYGON_STIPPLE                = 0x0B42,
	TW_EDGE_FLAG                      = 0x0B43,
	TW_CULL_FACE                      = 0x0B44,
	TW_CULL_FACE_MODE                 = 0x0B45,
	TW_FRONT_FACE                     = 0x0B46,
	TW_LIGHTING                       = 0x0B50,
	TW_LIGHT_MODEL_LOCAL_VIEWER       = 0x0B51,
	TW_LIGHT_MODEL_TWO_SIDE           = 0x0B52,
	TW_LIGHT_MODEL_AMBIENT            = 0x0B53,
	TW_SHADE_MODEL                    = 0x0B54,
	TW_COLOR_MATERIAL_FACE            = 0x0B55,
	TW_COLOR_MATERIAL_PARAMETER       = 0x0B56,
	TW_COLOR_MATERIAL                 = 0x0B57,
	TW_FOG                            = 0x0B60,
	TW_FOG_INDEX                      = 0x0B61,
	TW_FOG_DENSITY                    = 0x0B62,
	TW_FOG_START                      = 0x0B63,
	TW_FOG_END                        = 0x0B64,
	TW_FOG_MODE                       = 0x0B65,
	TW_FOG_COLOR                      = 0x0B66,
	TW_DEPTH_RANGE                    = 0x0B70,
	TW_DEPTH_TEST                     = 0x0B71,
	TW_DEPTH_WRITEMASK                = 0x0B72,
	TW_DEPTH_CLEAR_VALUE              = 0x0B73,
	TW_DEPTH_FUNC                     = 0x0B74,
	TW_ACCUM_CLEAR_VALUE              = 0x0B80,
	TW_STENCIL_TEST                   = 0x0B90,
	TW_STENCIL_CLEAR_VALUE            = 0x0B91,
	TW_STENCIL_FUNC                   = 0x0B92,
	TW_STENCIL_VALUE_MASK             = 0x0B93,
	TW_STENCIL_FAIL                   = 0x0B94,
	TW_STENCIL_PASS_DEPTH_FAIL        = 0x0B95,
	TW_STENCIL_PASS_DEPTH_PASS        = 0x0B96,
	TW_STENCIL_REF                    = 0x0B97,
	TW_STENCIL_WRITEMASK              = 0x0B98,
	TW_MATRIX_MODE                    = 0x0BA0,
	TW_NORMALIZE                      = 0x0BA1,
	TW_VIEWPORT                       = 0x0BA2,
	TW_MODELVIEW_STACK_DEPTH          = 0x0BA3,
	TW_PROJECTION_STACK_DEPTH         = 0x0BA4,
	TW_TEXTURE_STACK_DEPTH            = 0x0BA5,
	TW_MODELVIEW_MATRIX               = 0x0BA6,
	TW_PROJECTION_MATRIX              = 0x0BA7,
	TW_TEXTURE_MATRIX                 = 0x0BA8,
	TW_ATTRIB_STACK_DEPTH             = 0x0BB0,
	TW_CLIENT_ATTRIB_STACK_DEPTH      = 0x0BB1,
	TW_ALPHA_TEST                     = 0x0BC0,
	TW_ALPHA_TEST_FUNC                = 0x0BC1,
	TW_ALPHA_TEST_REF                 = 0x0BC2,
	TW_DITHER                         = 0x0BD0,
	TW_BLEND_DST                      = 0x0BE0,
	TW_BLEND_SRC                      = 0x0BE1,
	TW_BLEND                          = 0x0BE2,
	TW_LOGIC_OP_MODE                  = 0x0BF0,
	TW_INDEX_LOGIC_OP                 = 0x0BF1,
	TW_COLOR_LOGIC_OP                 = 0x0BF2,
	TW_AUX_BUFFERS                    = 0x0C00,
	TW_DRAW_BUFFER                    = 0x0C01,
	TW_READ_BUFFER                    = 0x0C02,
	TW_SCISSOR_BOX                    = 0x0C10,
	TW_SCISSOR_TEST                   = 0x0C11,
	TW_INDEX_CLEAR_VALUE              = 0x0C20,
	TW_INDEX_WRITEMASK                = 0x0C21,
	TW_COLOR_CLEAR_VALUE              = 0x0C22,
	TW_COLOR_WRITEMASK                = 0x0C23,
	TW_INDEX_MODE                     = 0x0C30,
	TW_RGBA_MODE                      = 0x0C31,
	TW_DOUBLEBUFFER                   = 0x0C32,
	TW_STEREO                         = 0x0C33,
	TW_RENDER_MODE                    = 0x0C40,
	TW_PERSPECTIVE_CORRECTION_HINT    = 0x0C50,
	TW_POINT_SMOOTH_HINT              = 0x0C51,
	TW_LINE_SMOOTH_HINT               = 0x0C52,
	TW_POLYGON_SMOOTH_HINT            = 0x0C53,
	TW_FOG_HINT                       = 0x0C54,
	TW_TEXTURE_GEN_S                  = 0x0C60,
	TW_TEXTURE_GEN_T                  = 0x0C61,
	TW_TEXTURE_GEN_R                  = 0x0C62,
	TW_TEXTURE_GEN_Q                  = 0x0C63,
	TW_PIXEL_MAP_I_TO_I               = 0x0C70,
	TW_PIXEL_MAP_S_TO_S               = 0x0C71,
	TW_PIXEL_MAP_I_TO_R               = 0x0C72,
	TW_PIXEL_MAP_I_TO_G               = 0x0C73,
	TW_PIXEL_MAP_I_TO_B               = 0x0C74,
	TW_PIXEL_MAP_I_TO_A               = 0x0C75,
	TW_PIXEL_MAP_R_TO_R               = 0x0C76,
	TW_PIXEL_MAP_G_TO_G               = 0x0C77,
	TW_PIXEL_MAP_B_TO_B               = 0x0C78,
	TW_PIXEL_MAP_A_TO_A               = 0x0C79,
	TW_PIXEL_MAP_I_TO_I_SIZE          = 0x0CB0,
	TW_PIXEL_MAP_S_TO_S_SIZE          = 0x0CB1,
	TW_PIXEL_MAP_I_TO_R_SIZE          = 0x0CB2,
	TW_PIXEL_MAP_I_TO_G_SIZE          = 0x0CB3,
	TW_PIXEL_MAP_I_TO_B_SIZE          = 0x0CB4,
	TW_PIXEL_MAP_I_TO_A_SIZE          = 0x0CB5,
	TW_PIXEL_MAP_R_TO_R_SIZE          = 0x0CB6,
	TW_PIXEL_MAP_G_TO_G_SIZE          = 0x0CB7,
	TW_PIXEL_MAP_B_TO_B_SIZE          = 0x0CB8,
	TW_PIXEL_MAP_A_TO_A_SIZE          = 0x0CB9,
	TW_UNPACK_SWAP_BYTES              = 0x0CF0,
	TW_UNPACK_LSB_FIRST               = 0x0CF1,
	TW_UNPACK_ROW_LENGTH              = 0x0CF2,
	TW_UNPACK_SKIP_ROWS               = 0x0CF3,
	TW_UNPACK_SKIP_PIXELS             = 0x0CF4,
	TW_UNPACK_ALIGNMENT               = 0x0CF5,
	TW_PACK_SWAP_BYTES                = 0x0D00,
	TW_PACK_LSB_FIRST                 = 0x0D01,
	TW_PACK_ROW_LENGTH                = 0x0D02,
	TW_PACK_SKIP_ROWS                 = 0x0D03,
	TW_PACK_SKIP_PIXELS               = 0x0D04,
	TW_PACK_ALIGNMENT                 = 0x0D05,
	TW_MAP_COLOR                      = 0x0D10,
	TW_MAP_STENCIL                    = 0x0D11,
	TW_INDEX_SHIFT                    = 0x0D12,
	TW_INDEX_OFFSET                   = 0x0D13,
	TW_RED_SCALE                      = 0x0D14,
	TW_RED_BIAS                       = 0x0D15,
	TW_ZOOM_X                         = 0x0D16,
	TW_ZOOM_Y                         = 0x0D17,
	TW_GREEN_SCALE                    = 0x0D18,
	TW_GREEN_BIAS                     = 0x0D19,
	TW_BLUE_SCALE                     = 0x0D1A,
	TW_BLUE_BIAS                      = 0x0D1B,
	TW_ALPHA_SCALE                    = 0x0D1C,
	TW_ALPHA_BIAS                     = 0x0D1D,
	TW_DEPTH_SCALE                    = 0x0D1E,
	TW_DEPTH_BIAS                     = 0x0D1F,
	TW_MAX_EVAL_ORDER                 = 0x0D30,
	TW_MAX_LIGHTS                     = 0x0D31,
	TW_MAX_CLIP_PLANES                = 0x0D32,
	TW_MAX_TEXTURE_SIZE               = 0x0D33,
	TW_MAX_PIXEL_MAP_TABLE            = 0x0D34,
	TW_MAX_ATTRIB_STACK_DEPTH         = 0x0D35,
	TW_MAX_MODELVIEW_STACK_DEPTH      = 0x0D36,
	TW_MAX_NAME_STACK_DEPTH           = 0x0D37,
	TW_MAX_PROJECTION_STACK_DEPTH     = 0x0D38,
	TW_MAX_TEXTURE_STACK_DEPTH        = 0x0D39,
	TW_MAX_VIEWPORT_DIMS              = 0x0D3A,
	TW_MAX_CLIENT_ATTRIB_STACK_DEPTH  = 0x0D3B,
	TW_SUBPIXEL_BITS                  = 0x0D50,
	TW_INDEX_BITS                     = 0x0D51,
	TW_RED_BITS                       = 0x0D52,
	TW_GREEN_BITS                     = 0x0D53,
	TW_BLUE_BITS                      = 0x0D54,
	TW_ALPHA_BITS                     = 0x0D55,
	TW_DEPTH_BITS                     = 0x0D56,
	TW_STENCIL_BITS                   = 0x0D57,
	TW_ACCUM_RED_BITS                 = 0x0D58,
	TW_ACCUM_GREEN_BITS               = 0x0D59,
	TW_ACCUM_BLUE_BITS                = 0x0D5A,
	TW_ACCUM_ALPHA_BITS               = 0x0D5B,
	TW_NAME_STACK_DEPTH               = 0x0D70,
	TW_AUTO_NORMAL                    = 0x0D80,
	TW_MAP1_COLOR_4                   = 0x0D90,
	TW_MAP1_INDEX                     = 0x0D91,
	TW_MAP1_NORMAL                    = 0x0D92,
	TW_MAP1_TEXTURE_COORD_1           = 0x0D93,
	TW_MAP1_TEXTURE_COORD_2           = 0x0D94,
	TW_MAP1_TEXTURE_COORD_3           = 0x0D95,
	TW_MAP1_TEXTURE_COORD_4           = 0x0D96,
	TW_MAP1_VERTEX_3                  = 0x0D97,
	TW_MAP1_VERTEX_4                  = 0x0D98,
	TW_MAP2_COLOR_4                   = 0x0DB0,
	TW_MAP2_INDEX                     = 0x0DB1,
	TW_MAP2_NORMAL                    = 0x0DB2,
	TW_MAP2_TEXTURE_COORD_1           = 0x0DB3,
	TW_MAP2_TEXTURE_COORD_2           = 0x0DB4,
	TW_MAP2_TEXTURE_COORD_3           = 0x0DB5,
	TW_MAP2_TEXTURE_COORD_4           = 0x0DB6,
	TW_MAP2_VERTEX_3                  = 0x0DB7,
	TW_MAP2_VERTEX_4                  = 0x0DB8,
	TW_MAP1_GRID_DOMAIN               = 0x0DD0,
	TW_MAP1_GRID_SEGMENTS             = 0x0DD1,
	TW_MAP2_GRID_DOMAIN               = 0x0DD2,
	TW_MAP2_GRID_SEGMENTS             = 0x0DD3,
	TW_TEXTURE_1D                     = 0x0DE0,
	TW_TEXTURE_2D                     = 0x0DE1,
	TW_FEEDBACK_BUFFER_POINTER        = 0x0DF0,
	TW_FEEDBACK_BUFFER_SIZE           = 0x0DF1,
	TW_FEEDBACK_BUFFER_TYPE           = 0x0DF2,
	TW_SELECTION_BUFFER_POINTER       = 0x0DF3,
	TW_SELECTION_BUFFER_SIZE          = 0x0DF4,

	// TextureParameterName
	TW_TEXTURE_MAG_FILTER = 0x2800,
	TW_TEXTURE_MIN_FILTER = 0x2801,
	TW_TEXTURE_WRAP_S     = 0x2802,
	TW_TEXTURE_WRAP_T     = 0x2803,

	// TextureWrapMode
	TW_CLAMP  = 0x2900,
	TW_REPEAT = 0x2901,

	// TextureMagFilter
	TW_NEAREST = 0x2600,
	TW_LINEAR  = 0x2601,

	// PixelCopyType
	TW_COLOR   = 0x1800,
	TW_DEPTH   = 0x1801,
	TW_STENCIL = 0x1802,

	// PixelFormat
	TW_COLOR_INDEX     = 0x1900,
	TW_STENCIL_INDEX   = 0x1901,
	TW_DEPTH_COMPONENT = 0x1902,
	TW_RED             = 0x1903,
	TW_GREEN           = 0x1904,
	TW_BLUE            = 0x1905,
	TW_ALPHA           = 0x1906,
	TW_RGB             = 0x1907,
	TW_RGBA            = 0x1908,
	TW_LUMINANCE       = 0x1909,
	TW_LUMINANCE_ALPHA = 0x190A,

	// DataType
	TW_BYTE           = 0x1400,
	TW_UNSIGNED_BYTE  = 0x1401,
	TW_SHORT          = 0x1402,
	TW_UNSIGNED_SHORT = 0x1403,
	TW_INT            = 0x1404,
	TW_UNSIGNED_INT   = 0x1405,
	TW_FLOAT          = 0x1406,
	TW_2_BYTES        = 0x1407,
	TW_3_BYTES        = 0x1408,
	TW_4_BYTES        = 0x1409,
	TW_DOUBLE         = 0x140A,

	// MaterialParameter
	TW_EMISSION            = 0x1600,
	TW_SHININESS           = 0x1601,
	TW_AMBIENT_AND_DIFFUSE = 0x1602,
	TW_COLOR_INDEXES       = 0x1603,

	// LightName
	TW_LIGHT0 = 0x4000,
	TW_LIGHT1 = 0x4001,
	TW_LIGHT2 = 0x4002,
	TW_LIGHT3 = 0x4003,
	TW_LIGHT4 = 0x4004,
	TW_LIGHT5 = 0x4005,
	TW_LIGHT6 = 0x4006,
	TW_LIGHT7 = 0x4007,

	// LightParameter
	TW_AMBIENT               = 0x1200,
	TW_DIFFUSE               = 0x1201,
	TW_SPECULAR              = 0x1202,
	TW_POSITION              = 0x1203,
	TW_SPOT_DIRECTION        = 0x1204,
	TW_SPOT_EXPONENT         = 0x1205,
	TW_SPOT_CUTOFF           = 0x1206,
	TW_CONSTANT_ATTENUATION  = 0x1207,
	TW_LINEAR_ATTENUATION    = 0x1208,
	TW_QUADRATIC_ATTENUATION = 0x1209,

	// HintMode
	TW_DONT_CARE = 0x1100,
	TW_FASTEST   = 0x1101,
	TW_NICEST    = 0x1102,

	// AlphaFunction
	TW_NEVER    = 0x0200,
	TW_LESS     = 0x0201,
	TW_EQUAL    = 0x0202,
	TW_LEQUAL   = 0x0203,
	TW_GREATER  = 0x0204,
	TW_NOTEQUAL = 0x0205,
	TW_GEQUAL   = 0x0206,
	TW_ALWAYS   = 0x0207,

	// MatrixMode
	TW_MODELVIEW  = 0x1700,
	TW_PROJECTION = 0x1701,
	TW_TEXTURE    = 0x1702,

	// vertex_array
	TW_VERTEX_ARRAY                = 0x8074,
	TW_NORMAL_ARRAY                = 0x8075,
	TW_COLOR_ARRAY                 = 0x8076,
	TW_INDEX_ARRAY                 = 0x8077,
	TW_TEXTURE_COORD_ARRAY         = 0x8078,
	TW_EDGE_FLAG_ARRAY             = 0x8079,
	TW_VERTEX_ARRAY_SIZE           = 0x807A,
	TW_VERTEX_ARRAY_TYPE           = 0x807B,
	TW_VERTEX_ARRAY_STRIDE         = 0x807C,
	TW_NORMAL_ARRAY_TYPE           = 0x807E,
	TW_NORMAL_ARRAY_STRIDE         = 0x807F,
	TW_COLOR_ARRAY_SIZE            = 0x8081,
	TW_COLOR_ARRAY_TYPE            = 0x8082,
	TW_COLOR_ARRAY_STRIDE          = 0x8083,
	TW_INDEX_ARRAY_TYPE            = 0x8085,
	TW_INDEX_ARRAY_STRIDE          = 0x8086,
	TW_TEXTURE_COORD_ARRAY_SIZE    = 0x8088,
	TW_TEXTURE_COORD_ARRAY_TYPE    = 0x8089,
	TW_TEXTURE_COORD_ARRAY_STRIDE  = 0x808A,
	TW_EDGE_FLAG_ARRAY_STRIDE      = 0x808C,
	TW_VERTEX_ARRAY_POINTER        = 0x808E,
	TW_NORMAL_ARRAY_POINTER        = 0x808F,
	TW_COLOR_ARRAY_POINTER         = 0x8090,
	TW_INDEX_ARRAY_POINTER         = 0x8091,
	TW_TEXTURE_COORD_ARRAY_POINTER = 0x8092,
	TW_EDGE_FLAG_ARRAY_POINTER     = 0x8093,
	TW_V2F                         = 0x2A20,
	TW_V3F                         = 0x2A21,
	TW_C4UB_V2F                    = 0x2A22,
	TW_C4UB_V3F                    = 0x2A23,
	TW_C3F_V3F                     = 0x2A24,
	TW_N3F_V3F                     = 0x2A25,
	TW_C4F_N3F_V3F                 = 0x2A26,
	TW_T2F_V3F                     = 0x2A27,
	TW_T4F_V4F                     = 0x2A28,
	TW_T2F_C4UB_V3F                = 0x2A29,
	TW_T2F_C3F_V3F                 = 0x2A2A,
	TW_T2F_N3F_V3F                 = 0x2A2B,
	TW_T2F_C4F_N3F_V3F             = 0x2A2C,
	TW_T4F_C4F_N3F_V4F             = 0x2A2D,

	// VBO Targets
	TW_ARRAY_BUFFER	        = 0x8892,
	TW_ELEMENT_ARRAY_BUFFER = 0x8893,

	// VBO Usage Hints
	TW_STREAM_DRAW	= 0x88E0,
	TW_STREAM_READ	= 0x88E1,
	TW_STREAM_COPY	= 0x88E2,
	TW_STATIC_DRAW	= 0x88E4,
	TW_STATIC_READ	= 0x88E5,
	TW_STATIC_COPY	= 0x88E6,
	TW_DYNAMIC_DRAW = 0x88E8,
	TW_DYNAMIC_READ = 0x88E9,
	TW_DYNAMIC_COPY = 0x88EA,

	// DAHA YAZILACAK ÇOK ŞEY VAR IMPL EDERKEN AKLIMA GELİR
} TwDictionary;

#define THREAD_SAFE
#define TW_PI 3.14159265358979323846f

/* === MATH === */
THREAD_SAFE void twMathIdentity(TwMat4* out_mat);
THREAD_SAFE void twMathOrtho(TwMat4* out_mat, TwFloat left, TwFloat right, TwFloat bottom, TwFloat top, TwFloat near_val, TwFloat far_val);
THREAD_SAFE void twMathPerspective(TwMat4* out_mat, TwFloat fov_y, TwFloat aspect, TwFloat near_val, TwFloat far_val);
THREAD_SAFE void twMathLookAt(TwMat4* out_mat, const TwVec3* eye, const TwVec3* center, const TwVec3* up);
THREAD_SAFE void twMathRotate(TwMat4* out_mat, TwFloat angle_deg, const TwFloat* axis);
THREAD_SAFE void twMathMultiply(TwMat4* out_mat, const TwMat4* mat_a, const TwMat4* mat_b);
THREAD_SAFE void twMathTranslate(TwMat4* out_mat, const TwFloat* xyz);
THREAD_SAFE void twMathScale(TwMat4* out_mat, const TwFloat* xyz);
THREAD_SAFE void twMathTransform(TwMat4* out_mat, const TwFloat* pos, TwFloat angle, const TwFloat* axis, const TwFloat* scale);
THREAD_SAFE TwFloat twMathLerp(TwFloat v0, TwFloat v1, TwFloat dt);

/* === INSTANCE === */

// bellekten yer ayırma ve bütün başlangıç ayarları ile yaratma
THREAD_SAFE TwInstance* twCreateInstance(const TwInstanceDesc* desc);
THREAD_SAFE TwBoolean twDeleteInstance(TwInstance* inst);
// texture vs. kaynaklar için pool allocator
THREAD_SAFE TwBoolean twInstanceAllocator(TwInstance* inst, TwDictionary type, TwUint count);

/* === WINDOW === */

// şuanlık pencere yaratma ve context açma, atama içinde bulunuyor
THREAD_SAFE TwBoolean twOpenWindow(TwInstance* inst);
THREAD_SAFE TwBoolean twIsWindowActive(TwInstance* inst);
THREAD_SAFE TwBoolean twCloseWindow(TwInstance* inst);

THREAD_SAFE void twSwapBuffer(TwInstance* inst);
THREAD_SAFE void twSwapInterval(TwInstance* inst, TwInt interval);
THREAD_SAFE void twWait(TwUint msec); // bu konuda şüphelerim var
THREAD_SAFE TwFloat twGetDeltaTime(TwInstance* inst);

THREAD_SAFE TwBoolean twKeyDown(TwInstance* inst, TwByte key);

/* === RENDER === */
	// FEATURE & ENVIRONMENT
	void twViewport(TwInstance* inst, TwInt x, TwInt y, TwSizei w, TwSizei h);
	void twScissor(TwInstance* inst, TwInt x, TwInt y, TwSizei w, TwSizei h);
	void twClearColor(TwInstance* inst, TwFloat red, TwFloat green, TwFloat blue, TwFloat alpha);
	void twClearDepth(TwInstance* inst, TwDouble d);
	void twClearStencil(TwInstance* inst, TwInt s);

	void twBlendFunc(TwInstance* inst, TwUint sfactor, TwUint dfactor);
	void twDepthFunc(TwInstance* inst, TwUint func);
	void twDepthMask(TwInstance* inst, TwBoolean flag);

	void twFrontFace(TwInstance* inst, TwUint mode);
	void twPolygonOffset(TwInstance* inst, TwFloat factor, TwFloat units);
	void twLineWidth(TwInstance* inst, TwFloat width);
	void twPointSize(TwInstance* inst, TwFloat size);

	// ASSET PIPELINE
	TwID twLoadTexture(TwInstance* inst, TwDictionary filter, TwDictionary wrap, const char* filepath);
	void twBindTextureIndex(TwInstance* inst, TwID texture_id);
		
	// BUFFER
	void twBindBuffer(TwInstance* inst, TwUint target, TwUint buffer);
	void twGenBuffers(TwInstance* inst, TwSizei n, TwUint* buffers);
	void twBufferData(TwInstance* inst, TwUint target, TwSizeiptr size, const void* data, TwUint usage);
	void twDeleteBuffers(TwInstance* inst, TwSizei n, const TwUint* buffers);
		
	// MATH & DISPATCH
	void twLoadProjectionMatrix(TwInstance* inst, const TwMat4* m);
	void twLoadModelViewMatrix(TwInstance* inst, const TwMat4* view, const TwMat4* model);
	void twInitCamera(TwInstance* inst, TwFloat fov, TwFloat aspect, TwFloat near_val, TwFloat far_val);
	void twCameraLookAt(TwInstance* inst, TwFloat* eye, TwFloat* center, TwFloat* up);
	void twCameraView(TwInstance* inst, TwMat4* out_mat);

	// DRAW
	void twClear(TwInstance* inst, TwUint mask);
	void twDrawArrays(TwInstance* inst, TwUint mode, TwInt first, TwSizei count);
	void twDrawElements(TwInstance* inst, TwUint mode, TwSizei count, TwUint type, const void* indices);
	void twInterleavedArrays(TwInstance* inst, TwUint format, const void* pointer);
	
	/*her an gidebilir*/void twDrawMesh(TwInstance* inst, TwID texture_id, TwUint format, TwUint mode, TwUint count, const void* pointer);
	void twDrawModel(TwInstance* inst, TwID model);

	// SHADER
	//glProgramLocalParameter4fARB
	//void glBindBufferARB (GLenum target, GLuint buffer);
	//void glDeleteBuffersARB(GLsizei n, const GLuint* buffers);
	//void glGenBuffersARB(GLsizei n, GLuint* buffers);
	//void glBufferDataARB(GLenum target, GLsizeiptrARB size, const void* data, GLenum usage);
	//void glBufferSubDataARB(GLenum target, GLintptrARB offset, GLsizeiptrARB size, const void* data);
	//void* glMapBufferARB(GLenum target, GLenum access);
	//GLboolean glUnmapBufferARB(GLenum target);
	//void glActiveTextureARB(GLenum texture);
	//void glClientActiveTextureARB(GLenum texture);
	TwID twLoadShaderARB(TwInstance* inst, const char* source);
	TwID twLoadShaderFileARB(TwInstance* inst, const char* filepath);
	void twBindShaderARB(TwInstance* inst, TwID shader);
	void twActiveTextureARB(TwInstance* inst, TwARB texture);

	// MISC
	TwID twLoadT3D(TwInstance* inst, const char* filepath);
	void twReadPixels(TwInstance* inst, TwInt x, TwInt y, TwSizei width, TwSizei height, TwUint format, TwUint type, void* pixels);

	// DEBUG
	TwUint twGetGpuCallCount(TwInstance* inst);
	void twResetGpuCallCount(TwInstance* inst);