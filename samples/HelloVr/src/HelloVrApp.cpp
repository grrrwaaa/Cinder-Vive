#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "cinder/Log.h"

#include "openvr.h"

struct Session {
	Session() { }
	~Session() {
		vr::VR_Shutdown();
	}
};

using namespace ci;
using namespace ci::app;
using namespace std;

class CGLRenderModel
{
public:
	CGLRenderModel( const std::string & sRenderModelName );
	~CGLRenderModel();

	bool BInit( const vr::RenderModel_t & vrModel, const vr::RenderModel_TextureMap_t & vrDiffuseTexture );
	void Cleanup();
	void Draw();
	const std::string & GetName() const { return m_sModelName; }

private:
	GLuint m_glVertBuffer;
	GLuint m_glIndexBuffer;
	GLuint m_glVertArray;
	GLuint m_glTexture;
	GLsizei m_unVertexCount;
	std::string m_sModelName;
};

class HelloVrApp : public App {
public:
	HelloVrApp();
	void mouseDown( MouseEvent event ) override;
	void update() override;
	void draw() override;
	void cleanup() override;
	void keyDown( KeyEvent event ) override;

	bool BInitGL();
	bool BInitCompositor();

	void SetupRenderModels();

	void ProcessVREvent( const vr::VREvent_t & event );

	bool SetupTexturemaps();

	void SetupScene();
	void AddCubeToScene( glm::mat4 mat, std::vector<float> &vertdata );
	void AddCubeVertex( float fl0, float fl1, float fl2, float fl3, float fl4, std::vector<float> &vertdata );

	void DrawControllers();

	bool SetupStereoRenderTargets();
	void SetupDistortion();
	void SetupCameras();

	void RenderStereoTargets();
	void RenderDistortion();
	void RenderScene( vr::Hmd_Eye nEye );

	glm::mat4 GetHMDMatrixProjectionEye( vr::Hmd_Eye nEye );
	glm::mat4 GetHMDMatrixPoseEye( vr::Hmd_Eye nEye );
	glm::mat4 GetCurrentViewProjectionMatrix( vr::Hmd_Eye nEye );
	void UpdateHMDMatrixPose();

	glm::mat4 ConvertSteamVRMatrixToMat4( const vr::HmdMatrix34_t &matPose );

	GLuint CompileGLShader( const char *pchShaderName, const char *pchVertexShader, const char *pchFragmentShader );
	bool CreateAllShaders();

	void SetupRenderModelForTrackedDevice( vr::TrackedDeviceIndex_t unTrackedDeviceIndex );
	
	CGLRenderModel *FindOrLoadRenderModel( const char *pchRenderModelName );

	static Session kSession;
private:
	bool m_bDebugOpenGL;
	bool m_bVerbose;
	bool m_bPerf;
	bool m_bVblank;
	bool m_bGlFinishHack;

	vr::IVRSystem *m_pHMD;
	vr::IVRRenderModels *m_pRenderModels;
	std::string m_strDriver;
	std::string m_strDisplay;
	vr::TrackedDevicePose_t m_rTrackedDevicePose[vr::k_unMaxTrackedDeviceCount];
	glm::mat4 m_rmat4DevicePose[vr::k_unMaxTrackedDeviceCount];
	bool m_rbShowTrackedDevice[vr::k_unMaxTrackedDeviceCount];

private:
	uint32_t m_nWindowWidth;
	uint32_t m_nWindowHeight;

private: // OpenGL bookkeeping
	int m_iTrackedControllerCount;
	int m_iTrackedControllerCount_Last;
	int m_iValidPoseCount;
	int m_iValidPoseCount_Last;
	bool m_bShowCubes;

	std::string m_strPoseClasses;                            // what classes we saw poses for this frame
	char m_rDevClassChar[vr::k_unMaxTrackedDeviceCount];   // for each device, a character representing its class

	int m_iSceneVolumeWidth;
	int m_iSceneVolumeHeight;
	int m_iSceneVolumeDepth;
	float m_fScaleSpacing;
	float m_fScale;

	int m_iSceneVolumeInit;                                  // if you want something other than the default 20x20x20

	float m_fNearClip;
	float m_fFarClip;

	gl::Texture2dRef mCubeTexture;

	unsigned int m_uiVertcount;

	GLuint m_glSceneVertBuffer;
	GLuint m_unSceneVAO;
	GLuint m_unLensVAO;
	GLuint m_glIDVertBuffer;
	GLuint m_glIDIndexBuffer;
	unsigned int m_uiIndexSize;

	GLuint m_glControllerVertBuffer;
	GLuint m_unControllerVAO;
	unsigned int m_uiControllerVertcount;

	glm::mat4 m_mat4HMDPose;
	glm::mat4 m_mat4eyePosLeft;
	glm::mat4 m_mat4eyePosRight;

	glm::mat4 m_mat4ProjectionCenter;
	glm::mat4 m_mat4ProjectionLeft;
	glm::mat4 m_mat4ProjectionRight;

	struct VertexDataScene
	{
		glm::vec3 position;
		glm::vec2 texCoord;
	};

	struct VertexDataLens
	{
		glm::vec2 position;
		glm::vec2 texCoordRed;
		glm::vec2 texCoordGreen;
		glm::vec2 texCoordBlue;
	};

	GLuint m_unSceneProgramID;
	GLuint m_unLensProgramID;
	GLuint m_unControllerTransformProgramID;
	GLuint m_unRenderModelProgramID;

	GLint m_nSceneMatrixLocation;
	GLint m_nControllerMatrixLocation;
	GLint m_nRenderModelMatrixLocation;

	struct FramebufferDesc
	{
		GLuint m_nDepthBufferId;
		GLuint m_nRenderTextureId;
		GLuint m_nRenderFramebufferId;
		GLuint m_nResolveTextureId;
		GLuint m_nResolveFramebufferId;
	};
	FramebufferDesc leftEyeDesc;
	FramebufferDesc rightEyeDesc;

	bool CreateFrameBuffer( int nWidth, int nHeight, FramebufferDesc &framebufferDesc );

	uint32_t m_nRenderWidth;
	uint32_t m_nRenderHeight;

	std::vector< CGLRenderModel * > m_vecRenderModels;
	CGLRenderModel *m_rTrackedDeviceToRenderModel[vr::k_unMaxTrackedDeviceCount];
};

//-----------------------------------------------------------------------------
// Purpose: Helper to get a string from a tracked device property and turn it
//			into a std::string
//-----------------------------------------------------------------------------
std::string GetTrackedDeviceString( vr::IVRSystem *pHmd, vr::TrackedDeviceIndex_t unDevice, vr::TrackedDeviceProperty prop, vr::TrackedPropertyError *peError = NULL )
{
	uint32_t unRequiredBufferLen = pHmd->GetStringTrackedDeviceProperty( unDevice, prop, NULL, 0, peError );
	if( unRequiredBufferLen == 0 )
		return "";

	char *pchBuffer = new char[unRequiredBufferLen];
	unRequiredBufferLen = pHmd->GetStringTrackedDeviceProperty( unDevice, prop, pchBuffer, unRequiredBufferLen, peError );
	std::string sResult = pchBuffer;
	delete[] pchBuffer;
	return sResult;
}

HelloVrApp::HelloVrApp()
	: m_nWindowWidth( 1280 )
	, m_nWindowHeight( 720 )
	, m_unSceneProgramID( 0 )
	, m_unLensProgramID( 0 )
	, m_unControllerTransformProgramID( 0 )
	, m_unRenderModelProgramID( 0 )
	, m_pHMD( NULL )
	, m_pRenderModels( NULL )
	, m_bDebugOpenGL( false )
	, m_bVerbose( false )
	, m_bPerf( false )
	, m_bVblank( false )
	, m_bGlFinishHack( true )
	, m_glControllerVertBuffer( 0 )
	, m_unControllerVAO( 0 )
	, m_unLensVAO( 0 )
	, m_unSceneVAO( 0 )
	, m_nSceneMatrixLocation( -1 )
	, m_nControllerMatrixLocation( -1 )
	, m_nRenderModelMatrixLocation( -1 )
	, m_iTrackedControllerCount( 0 )
	, m_iTrackedControllerCount_Last( -1 )
	, m_iValidPoseCount( 0 )
	, m_iValidPoseCount_Last( -1 )
	, m_iSceneVolumeInit( 20 )
	, m_strPoseClasses( "" )
	, m_bShowCubes( true )
{
	// other initialization tasks are done in BInit
	memset( m_rDevClassChar, 0, sizeof( m_rDevClassChar ) );

	gl::enableVerticalSync( false );

	// Loading the SteamVR Runtime
	vr::EVRInitError eError = vr::VRInitError_None;
	m_pHMD = vr::VR_Init( &eError, vr::VRApplication_Scene );

	if( eError != vr::VRInitError_None ) {
		m_pHMD = NULL;
		CI_LOG_E( "Unable to init VR runtime: " << vr::VR_GetVRInitErrorAsEnglishDescription( eError ) );
		quit();
	}


	m_pRenderModels = (vr::IVRRenderModels *)vr::VR_GetGenericInterface( vr::IVRRenderModels_Version, &eError );
	if( !m_pRenderModels ) {
		m_pHMD = NULL;
		vr::VR_Shutdown();

		CI_LOG_E( "Unable to get render model interface: " << vr::VR_GetVRInitErrorAsEnglishDescription( eError ) );
		quit();
	}

	int nWindowPosX = 700;
	int nWindowPosY = 100;
	m_nWindowWidth = 1280;
	m_nWindowHeight = 720;



	m_strDriver = "No Driver";
	m_strDisplay = "No Display";

	m_strDriver = GetTrackedDeviceString( m_pHMD, vr::k_unTrackedDeviceIndex_Hmd, vr::Prop_TrackingSystemName_String );
	m_strDisplay = GetTrackedDeviceString( m_pHMD, vr::k_unTrackedDeviceIndex_Hmd, vr::Prop_SerialNumber_String );

	std::string strWindowTitle = "hellovr_sdl - " + m_strDriver + " " + m_strDisplay;
	getWindow()->setTitle( strWindowTitle );

	// cube array
	m_iSceneVolumeWidth = m_iSceneVolumeInit;
	m_iSceneVolumeHeight = m_iSceneVolumeInit;
	m_iSceneVolumeDepth = m_iSceneVolumeInit;

	m_fScale = 0.3f;
	m_fScaleSpacing = 4.0f;

	m_fNearClip = 0.1f;
	m_fFarClip = 30.0f;

	m_uiVertcount = 0;

	if( !BInitGL() ) {
		CI_LOG_E( "Unable to initialize OpenGL!" );
		quit();
	}

	if( !BInitCompositor() ) {
		CI_LOG_E( "Failed to initialize VR Compositor!" );
		quit();
	}
};

void APIENTRY DebugCallback( GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const char* message, const void* userParam )
{
	CI_LOG_I( "GL Error: " << message );
}

bool HelloVrApp::BInitGL()
{
	if( m_bDebugOpenGL )
	{
		glDebugMessageCallback( DebugCallback, nullptr );
		glDebugMessageControl( GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE );
		glEnable( GL_DEBUG_OUTPUT_SYNCHRONOUS );
	}

	if( !CreateAllShaders() )
		return false;

	SetupTexturemaps();
	SetupScene();
	SetupCameras();
	SetupStereoRenderTargets();
	SetupDistortion();

	SetupRenderModels();

	return true;
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool HelloVrApp::BInitCompositor()
{
	vr::EVRInitError peError = vr::VRInitError_None;

	if( !vr::VRCompositor() )
	{
		printf( "Compositor initialization failed. See log file for details\n" );
		return false;
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Processes a single VR event
//-----------------------------------------------------------------------------
void HelloVrApp::ProcessVREvent( const vr::VREvent_t & event )
{
	switch( event.eventType )
	{
	case vr::VREvent_TrackedDeviceActivated:
	{
		SetupRenderModelForTrackedDevice( event.trackedDeviceIndex );
		CI_LOG_I( "Device %u attached. Setting up render model.\n", event.trackedDeviceIndex );
	}
	break;
	case vr::VREvent_TrackedDeviceDeactivated:
	{
		CI_LOG_I( "Device %u detached.\n", event.trackedDeviceIndex );
	}
	break;
	case vr::VREvent_TrackedDeviceUpdated:
	{
		CI_LOG_I( "Device %u updated.\n", event.trackedDeviceIndex );
	}
	break;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Compiles a GL shader program and returns the handle. Returns 0 if
//			the shader couldn't be compiled for some reason.
//-----------------------------------------------------------------------------
GLuint HelloVrApp::CompileGLShader( const char *pchShaderName, const char *pchVertexShader, const char *pchFragmentShader )
{
	GLuint unProgramID = glCreateProgram();

	GLuint nSceneVertexShader = glCreateShader( GL_VERTEX_SHADER );
	glShaderSource( nSceneVertexShader, 1, &pchVertexShader, NULL );
	glCompileShader( nSceneVertexShader );

	GLint vShaderCompiled = GL_FALSE;
	glGetShaderiv( nSceneVertexShader, GL_COMPILE_STATUS, &vShaderCompiled );
	if( vShaderCompiled != GL_TRUE )
	{
		CI_LOG_I( "%s - Unable to compile vertex shader %d!\n", pchShaderName, nSceneVertexShader );
		glDeleteProgram( unProgramID );
		glDeleteShader( nSceneVertexShader );
		return 0;
	}
	glAttachShader( unProgramID, nSceneVertexShader );
	glDeleteShader( nSceneVertexShader ); // the program hangs onto this once it's attached

	GLuint  nSceneFragmentShader = glCreateShader( GL_FRAGMENT_SHADER );
	glShaderSource( nSceneFragmentShader, 1, &pchFragmentShader, NULL );
	glCompileShader( nSceneFragmentShader );

	GLint fShaderCompiled = GL_FALSE;
	glGetShaderiv( nSceneFragmentShader, GL_COMPILE_STATUS, &fShaderCompiled );
	if( fShaderCompiled != GL_TRUE )
	{
		CI_LOG_I( "%s - Unable to compile fragment shader %d!\n", pchShaderName, nSceneFragmentShader );
		glDeleteProgram( unProgramID );
		glDeleteShader( nSceneFragmentShader );
		return 0;
	}

	glAttachShader( unProgramID, nSceneFragmentShader );
	glDeleteShader( nSceneFragmentShader ); // the program hangs onto this once it's attached

	glLinkProgram( unProgramID );

	GLint programSuccess = GL_TRUE;
	glGetProgramiv( unProgramID, GL_LINK_STATUS, &programSuccess );
	if( programSuccess != GL_TRUE )
	{
		CI_LOG_I( "%s - Error linking program %d!\n", pchShaderName, unProgramID );
		glDeleteProgram( unProgramID );
		return 0;
	}

	glUseProgram( unProgramID );
	glUseProgram( 0 );

	return unProgramID;
}


//-----------------------------------------------------------------------------
// Purpose: Creates all the shaders used by HelloVR SDL
//-----------------------------------------------------------------------------
bool HelloVrApp::CreateAllShaders()
{
	m_unSceneProgramID = CompileGLShader(
		"Scene",

		// Vertex Shader
		"#version 410\n"
		"uniform mat4 matrix;\n"
		"layout(location = 0) in vec4 position;\n"
		"layout(location = 1) in vec2 v2UVcoordsIn;\n"
		"layout(location = 2) in vec3 v3NormalIn;\n"
		"out vec2 v2UVcoords;\n"
		"void main()\n"
		"{\n"
		"	v2UVcoords = v2UVcoordsIn;\n"
		"	gl_Position = matrix * position;\n"
		"}\n",

		// Fragment Shader
		"#version 410 core\n"
		"uniform sampler2D mytexture;\n"
		"in vec2 v2UVcoords;\n"
		"out vec4 outputColor;\n"
		"void main()\n"
		"{\n"
		"   outputColor = texture(mytexture, v2UVcoords);\n"
		"}\n"
		);
	m_nSceneMatrixLocation = glGetUniformLocation( m_unSceneProgramID, "matrix" );
	if( m_nSceneMatrixLocation == -1 )
	{
		CI_LOG_E( "Unable to find matrix uniform in scene shader\n" );
		return false;
	}

	m_unControllerTransformProgramID = CompileGLShader(
		"Controller",

		// vertex shader
		"#version 410\n"
		"uniform mat4 matrix;\n"
		"layout(location = 0) in vec4 position;\n"
		"layout(location = 1) in vec3 v3ColorIn;\n"
		"out vec4 v4Color;\n"
		"void main()\n"
		"{\n"
		"	v4Color.xyz = v3ColorIn; v4Color.a = 1.0;\n"
		"	gl_Position = matrix * position;\n"
		"}\n",

		// fragment shader
		"#version 410\n"
		"in vec4 v4Color;\n"
		"out vec4 outputColor;\n"
		"void main()\n"
		"{\n"
		"   outputColor = v4Color;\n"
		"}\n"
		);
	m_nControllerMatrixLocation = glGetUniformLocation( m_unControllerTransformProgramID, "matrix" );
	if( m_nControllerMatrixLocation == -1 )
	{
		CI_LOG_E( "Unable to find matrix uniform in controller shader\n" );
		return false;
	}

	m_unRenderModelProgramID = CompileGLShader(
		"render model",

		// vertex shader
		"#version 410\n"
		"uniform mat4 matrix;\n"
		"layout(location = 0) in vec4 position;\n"
		"layout(location = 1) in vec3 v3NormalIn;\n"
		"layout(location = 2) in vec2 v2TexCoordsIn;\n"
		"out vec2 v2TexCoord;\n"
		"void main()\n"
		"{\n"
		"	v2TexCoord = v2TexCoordsIn;\n"
		"	gl_Position = matrix * vec4(position.xyz, 1);\n"
		"}\n",

		//fragment shader
		"#version 410 core\n"
		"uniform sampler2D diffuse;\n"
		"in vec2 v2TexCoord;\n"
		"out vec4 outputColor;\n"
		"void main()\n"
		"{\n"
		"   outputColor = texture( diffuse, v2TexCoord);\n"
		"}\n"

		);
	m_nRenderModelMatrixLocation = glGetUniformLocation( m_unRenderModelProgramID, "matrix" );
	if( m_nRenderModelMatrixLocation == -1 )
	{
		CI_LOG_E( "Unable to find matrix uniform in render model shader\n" );
		return false;
	}

	m_unLensProgramID = CompileGLShader(
		"Distortion",

		// vertex shader
		"#version 410 core\n"
		"layout(location = 0) in vec4 position;\n"
		"layout(location = 1) in vec2 v2UVredIn;\n"
		"layout(location = 2) in vec2 v2UVGreenIn;\n"
		"layout(location = 3) in vec2 v2UVblueIn;\n"
		"noperspective  out vec2 v2UVred;\n"
		"noperspective  out vec2 v2UVgreen;\n"
		"noperspective  out vec2 v2UVblue;\n"
		"void main()\n"
		"{\n"
		"	v2UVred = v2UVredIn;\n"
		"	v2UVgreen = v2UVGreenIn;\n"
		"	v2UVblue = v2UVblueIn;\n"
		"	gl_Position = position;\n"
		"}\n",

		// fragment shader
		"#version 410 core\n"
		"uniform sampler2D mytexture;\n"

		"noperspective  in vec2 v2UVred;\n"
		"noperspective  in vec2 v2UVgreen;\n"
		"noperspective  in vec2 v2UVblue;\n"

		"out vec4 outputColor;\n"

		"void main()\n"
		"{\n"
		"	float fBoundsCheck = ( (dot( vec2( lessThan( v2UVgreen.xy, vec2(0.05, 0.05)) ), vec2(1.0, 1.0))+dot( vec2( greaterThan( v2UVgreen.xy, vec2( 0.95, 0.95)) ), vec2(1.0, 1.0))) );\n"
		"	if( fBoundsCheck > 1.0 )\n"
		"	{ outputColor = vec4( 0, 0, 0, 1.0 ); }\n"
		"	else\n"
		"	{\n"
		"		float red = texture(mytexture, v2UVred).x;\n"
		"		float green = texture(mytexture, v2UVgreen).y;\n"
		"		float blue = texture(mytexture, v2UVblue).z;\n"
		"		outputColor = vec4( red, green, blue, 1.0  ); }\n"
		"}\n"
		);


	return m_unSceneProgramID != 0
		&& m_unControllerTransformProgramID != 0
		&& m_unRenderModelProgramID != 0
		&& m_unLensProgramID != 0;
}

bool HelloVrApp::SetupTexturemaps()
{
	GLfloat fLargest;
	glGetFloatv( GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &fLargest );
	gl::Texture2d::Format fmt;
	fmt.setMaxAnisotropy( fLargest );
	fmt.mipmap( true );
	fmt.loadTopDown();
	mCubeTexture = gl::Texture2d::create( loadImage( loadAsset( "cube_texture.png" ) ), fmt );
	return true;
}


//-----------------------------------------------------------------------------
// Purpose: create a sea of cubes
//-----------------------------------------------------------------------------
void HelloVrApp::SetupScene()
{
	if( !m_pHMD )
		return;

	std::vector<float> vertdataarray;

	glm::mat4 matScale = glm::scale( vec3( m_fScale ) );
	glm::mat4 matTransform = glm::translate( vec3(
		-((float)m_iSceneVolumeWidth * m_fScaleSpacing) / 2.f,
		-((float)m_iSceneVolumeHeight * m_fScaleSpacing) / 2.f,
		-((float)m_iSceneVolumeDepth * m_fScaleSpacing) / 2.f ) );

	glm::mat4 mat = matScale * matTransform;

	for( int z = 0; z< m_iSceneVolumeDepth; z++ )
	{
		for( int y = 0; y< m_iSceneVolumeHeight; y++ )
		{
			for( int x = 0; x< m_iSceneVolumeWidth; x++ )
			{
				AddCubeToScene( mat, vertdataarray );
				mat = mat * glm::translate( vec3( m_fScaleSpacing, 0, 0 ) );
			}
			mat = mat * glm::translate( vec3( -((float)m_iSceneVolumeWidth) * m_fScaleSpacing, m_fScaleSpacing, 0 ) );
		}
		mat = mat * glm::translate( vec3( 0, -((float)m_iSceneVolumeHeight) * m_fScaleSpacing, m_fScaleSpacing ) );
	}
	m_uiVertcount = vertdataarray.size() / 5;

	glGenVertexArrays( 1, &m_unSceneVAO );
	glBindVertexArray( m_unSceneVAO );

	glGenBuffers( 1, &m_glSceneVertBuffer );
	glBindBuffer( GL_ARRAY_BUFFER, m_glSceneVertBuffer );
	glBufferData( GL_ARRAY_BUFFER, sizeof( float ) * vertdataarray.size(), &vertdataarray[0], GL_STATIC_DRAW );

	glBindBuffer( GL_ARRAY_BUFFER, m_glSceneVertBuffer );

	GLsizei stride = sizeof( VertexDataScene );
	uintptr_t offset = 0;

	glEnableVertexAttribArray( 0 );
	glVertexAttribPointer( 0, 3, GL_FLOAT, GL_FALSE, stride, (const void *)offset );

	offset += sizeof( glm::vec3 );
	glEnableVertexAttribArray( 1 );
	glVertexAttribPointer( 1, 2, GL_FLOAT, GL_FALSE, stride, (const void *)offset );

	glBindVertexArray( 0 );
	glDisableVertexAttribArray( 0 );
	glDisableVertexAttribArray( 1 );

}

void HelloVrApp::AddCubeVertex( float fl0, float fl1, float fl2, float fl3, float fl4, std::vector<float> &vertdata )
{
	vertdata.push_back( fl0 );
	vertdata.push_back( fl1 );
	vertdata.push_back( fl2 );
	vertdata.push_back( fl3 );
	vertdata.push_back( fl4 );
}

void HelloVrApp::AddCubeToScene( glm::mat4 mat, std::vector<float> &vertdata )
{
	// glm::mat4 mat( outermat.data() );

	vec4 A = mat * vec4( 0, 0, 0, 1 );
	vec4 B = mat * vec4( 1, 0, 0, 1 );
	vec4 C = mat * vec4( 1, 1, 0, 1 );
	vec4 D = mat * vec4( 0, 1, 0, 1 );
	vec4 E = mat * vec4( 0, 0, 1, 1 );
	vec4 F = mat * vec4( 1, 0, 1, 1 );
	vec4 G = mat * vec4( 1, 1, 1, 1 );
	vec4 H = mat * vec4( 0, 1, 1, 1 );

	// triangles instead of quads
	AddCubeVertex( E.x, E.y, E.z, 0, 1, vertdata ); //Front
	AddCubeVertex( F.x, F.y, F.z, 1, 1, vertdata );
	AddCubeVertex( G.x, G.y, G.z, 1, 0, vertdata );
	AddCubeVertex( G.x, G.y, G.z, 1, 0, vertdata );
	AddCubeVertex( H.x, H.y, H.z, 0, 0, vertdata );
	AddCubeVertex( E.x, E.y, E.z, 0, 1, vertdata );

	AddCubeVertex( B.x, B.y, B.z, 0, 1, vertdata ); //Back
	AddCubeVertex( A.x, A.y, A.z, 1, 1, vertdata );
	AddCubeVertex( D.x, D.y, D.z, 1, 0, vertdata );
	AddCubeVertex( D.x, D.y, D.z, 1, 0, vertdata );
	AddCubeVertex( C.x, C.y, C.z, 0, 0, vertdata );
	AddCubeVertex( B.x, B.y, B.z, 0, 1, vertdata );

	AddCubeVertex( H.x, H.y, H.z, 0, 1, vertdata ); //Top
	AddCubeVertex( G.x, G.y, G.z, 1, 1, vertdata );
	AddCubeVertex( C.x, C.y, C.z, 1, 0, vertdata );
	AddCubeVertex( C.x, C.y, C.z, 1, 0, vertdata );
	AddCubeVertex( D.x, D.y, D.z, 0, 0, vertdata );
	AddCubeVertex( H.x, H.y, H.z, 0, 1, vertdata );

	AddCubeVertex( A.x, A.y, A.z, 0, 1, vertdata ); //Bottom
	AddCubeVertex( B.x, B.y, B.z, 1, 1, vertdata );
	AddCubeVertex( F.x, F.y, F.z, 1, 0, vertdata );
	AddCubeVertex( F.x, F.y, F.z, 1, 0, vertdata );
	AddCubeVertex( E.x, E.y, E.z, 0, 0, vertdata );
	AddCubeVertex( A.x, A.y, A.z, 0, 1, vertdata );

	AddCubeVertex( A.x, A.y, A.z, 0, 1, vertdata ); //Left
	AddCubeVertex( E.x, E.y, E.z, 1, 1, vertdata );
	AddCubeVertex( H.x, H.y, H.z, 1, 0, vertdata );
	AddCubeVertex( H.x, H.y, H.z, 1, 0, vertdata );
	AddCubeVertex( D.x, D.y, D.z, 0, 0, vertdata );
	AddCubeVertex( A.x, A.y, A.z, 0, 1, vertdata );

	AddCubeVertex( F.x, F.y, F.z, 0, 1, vertdata ); //Right
	AddCubeVertex( B.x, B.y, B.z, 1, 1, vertdata );
	AddCubeVertex( C.x, C.y, C.z, 1, 0, vertdata );
	AddCubeVertex( C.x, C.y, C.z, 1, 0, vertdata );
	AddCubeVertex( G.x, G.y, G.z, 0, 0, vertdata );
	AddCubeVertex( F.x, F.y, F.z, 0, 1, vertdata );
}


//-----------------------------------------------------------------------------
// Purpose: Draw all of the controllers as X/Y/Z lines
//-----------------------------------------------------------------------------
void HelloVrApp::DrawControllers()
{
	// don't draw controllers if somebody else has input focus
	if( m_pHMD->IsInputFocusCapturedByAnotherProcess() )
		return;

	std::vector<float> vertdataarray;

	m_uiControllerVertcount = 0;
	m_iTrackedControllerCount = 0;

	for( vr::TrackedDeviceIndex_t unTrackedDevice = vr::k_unTrackedDeviceIndex_Hmd + 1; unTrackedDevice < vr::k_unMaxTrackedDeviceCount; ++unTrackedDevice )
	{
		if( !m_pHMD->IsTrackedDeviceConnected( unTrackedDevice ) )
			continue;

		if( m_pHMD->GetTrackedDeviceClass( unTrackedDevice ) != vr::TrackedDeviceClass_Controller )
			continue;

		m_iTrackedControllerCount += 1;

		if( !m_rTrackedDevicePose[unTrackedDevice].bPoseIsValid )
			continue;

		const glm::mat4 & mat = m_rmat4DevicePose[unTrackedDevice];

		glm::vec4 center = mat * glm::vec4( 0, 0, 0, 1 );

		for( int i = 0; i < 3; ++i )
		{
			glm::vec3 color( 0, 0, 0 );
			glm::vec4 point( 0, 0, 0, 1 );
			point[i] += 0.05f;  // offset in X, Y, Z
			color[i] = 1.0;  // R, G, B
			point = mat * point;
			vertdataarray.push_back( center.x );
			vertdataarray.push_back( center.y );
			vertdataarray.push_back( center.z );

			vertdataarray.push_back( color.x );
			vertdataarray.push_back( color.y );
			vertdataarray.push_back( color.z );

			vertdataarray.push_back( point.x );
			vertdataarray.push_back( point.y );
			vertdataarray.push_back( point.z );

			vertdataarray.push_back( color.x );
			vertdataarray.push_back( color.y );
			vertdataarray.push_back( color.z );

			m_uiControllerVertcount += 2;
		}

		glm::vec4 start = mat * glm::vec4( 0, 0, -0.02f, 1 );
		glm::vec4 end = mat * glm::vec4( 0, 0, -39.f, 1 );
		glm::vec3 color( .92f, .92f, .71f );

		vertdataarray.push_back( start.x ); vertdataarray.push_back( start.y ); vertdataarray.push_back( start.z );
		vertdataarray.push_back( color.x ); vertdataarray.push_back( color.y ); vertdataarray.push_back( color.z );

		vertdataarray.push_back( end.x ); vertdataarray.push_back( end.y ); vertdataarray.push_back( end.z );
		vertdataarray.push_back( color.x ); vertdataarray.push_back( color.y ); vertdataarray.push_back( color.z );
		m_uiControllerVertcount += 2;
	}

	// Setup the VAO the first time through.
	if( m_unControllerVAO == 0 )
	{
		glGenVertexArrays( 1, &m_unControllerVAO );
		glBindVertexArray( m_unControllerVAO );

		glGenBuffers( 1, &m_glControllerVertBuffer );
		glBindBuffer( GL_ARRAY_BUFFER, m_glControllerVertBuffer );

		GLuint stride = 2 * 3 * sizeof( float );
		GLuint offset = 0;

		glEnableVertexAttribArray( 0 );
		glVertexAttribPointer( 0, 3, GL_FLOAT, GL_FALSE, stride, (const void *)offset );

		offset += sizeof( glm::vec3 );
		glEnableVertexAttribArray( 1 );
		glVertexAttribPointer( 1, 3, GL_FLOAT, GL_FALSE, stride, (const void *)offset );

		glBindVertexArray( 0 );
	}

	glBindBuffer( GL_ARRAY_BUFFER, m_glControllerVertBuffer );

	// set vertex data if we have some
	if( vertdataarray.size() > 0 )
	{
		//$ TODO: Use glBufferSubData for this...
		glBufferData( GL_ARRAY_BUFFER, sizeof( float ) * vertdataarray.size(), &vertdataarray[0], GL_STREAM_DRAW );
	}
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void HelloVrApp::SetupCameras()
{
	m_mat4ProjectionLeft = GetHMDMatrixProjectionEye( vr::Eye_Left );
	m_mat4ProjectionRight = GetHMDMatrixProjectionEye( vr::Eye_Right );
	m_mat4eyePosLeft = GetHMDMatrixPoseEye( vr::Eye_Left );
	m_mat4eyePosRight = GetHMDMatrixPoseEye( vr::Eye_Right );
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool HelloVrApp::CreateFrameBuffer( int nWidth, int nHeight, FramebufferDesc &framebufferDesc )
{
	glGenFramebuffers( 1, &framebufferDesc.m_nRenderFramebufferId );
	glBindFramebuffer( GL_FRAMEBUFFER, framebufferDesc.m_nRenderFramebufferId );

	glGenRenderbuffers( 1, &framebufferDesc.m_nDepthBufferId );
	glBindRenderbuffer( GL_RENDERBUFFER, framebufferDesc.m_nDepthBufferId );
	glRenderbufferStorageMultisample( GL_RENDERBUFFER, 4, GL_DEPTH_COMPONENT, nWidth, nHeight );
	glFramebufferRenderbuffer( GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, framebufferDesc.m_nDepthBufferId );

	glGenTextures( 1, &framebufferDesc.m_nRenderTextureId );
	glBindTexture( GL_TEXTURE_2D_MULTISAMPLE, framebufferDesc.m_nRenderTextureId );
	glTexImage2DMultisample( GL_TEXTURE_2D_MULTISAMPLE, 4, GL_RGBA8, nWidth, nHeight, true );
	glFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE, framebufferDesc.m_nRenderTextureId, 0 );

	glGenFramebuffers( 1, &framebufferDesc.m_nResolveFramebufferId );
	glBindFramebuffer( GL_FRAMEBUFFER, framebufferDesc.m_nResolveFramebufferId );

	glGenTextures( 1, &framebufferDesc.m_nResolveTextureId );
	glBindTexture( GL_TEXTURE_2D, framebufferDesc.m_nResolveTextureId );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0 );
	glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA8, nWidth, nHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr );
	glFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, framebufferDesc.m_nResolveTextureId, 0 );

	// check FBO status
	GLenum status = glCheckFramebufferStatus( GL_FRAMEBUFFER );
	if( status != GL_FRAMEBUFFER_COMPLETE )
	{
		return false;
	}

	glBindFramebuffer( GL_FRAMEBUFFER, 0 );

	return true;
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool HelloVrApp::SetupStereoRenderTargets()
{
	if( !m_pHMD )
		return false;

	m_pHMD->GetRecommendedRenderTargetSize( &m_nRenderWidth, &m_nRenderHeight );

	CreateFrameBuffer( m_nRenderWidth, m_nRenderHeight, leftEyeDesc );
	CreateFrameBuffer( m_nRenderWidth, m_nRenderHeight, rightEyeDesc );

	return true;
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void HelloVrApp::SetupDistortion()
{
	if( !m_pHMD )
		return;

	GLushort m_iLensGridSegmentCountH = 43;
	GLushort m_iLensGridSegmentCountV = 43;

	float w = (float)(1.0 / float( m_iLensGridSegmentCountH - 1 ));
	float h = (float)(1.0 / float( m_iLensGridSegmentCountV - 1 ));

	float u, v = 0;

	std::vector<VertexDataLens> vVerts( 0 );
	VertexDataLens vert;

	//left eye distortion verts
	float Xoffset = -1;
	for( int y = 0; y<m_iLensGridSegmentCountV; y++ )
	{
		for( int x = 0; x<m_iLensGridSegmentCountH; x++ )
		{
			u = x*w; v = 1 - y*h;
			vert.position = glm::vec2( Xoffset + u, -1 + 2 * y*h );

			vr::DistortionCoordinates_t dc0 = m_pHMD->ComputeDistortion( vr::Eye_Left, u, v );

			vert.texCoordRed = glm::vec2( dc0.rfRed[0], 1 - dc0.rfRed[1] );
			vert.texCoordGreen = glm::vec2( dc0.rfGreen[0], 1 - dc0.rfGreen[1] );
			vert.texCoordBlue = glm::vec2( dc0.rfBlue[0], 1 - dc0.rfBlue[1] );

			vVerts.push_back( vert );
		}
	}

	//right eye distortion verts
	Xoffset = 0;
	for( int y = 0; y<m_iLensGridSegmentCountV; y++ )
	{
		for( int x = 0; x<m_iLensGridSegmentCountH; x++ )
		{
			u = x*w; v = 1 - y*h;
			vert.position = glm::vec2( Xoffset + u, -1 + 2 * y*h );

			vr::DistortionCoordinates_t dc0 = m_pHMD->ComputeDistortion( vr::Eye_Right, u, v );

			vert.texCoordRed = glm::vec2( dc0.rfRed[0], 1 - dc0.rfRed[1] );
			vert.texCoordGreen = glm::vec2( dc0.rfGreen[0], 1 - dc0.rfGreen[1] );
			vert.texCoordBlue = glm::vec2( dc0.rfBlue[0], 1 - dc0.rfBlue[1] );

			vVerts.push_back( vert );
		}
	}

	std::vector<GLushort> vIndices;
	GLushort a, b, c, d;

	GLushort offset = 0;
	for( GLushort y = 0; y<m_iLensGridSegmentCountV - 1; y++ )
	{
		for( GLushort x = 0; x<m_iLensGridSegmentCountH - 1; x++ )
		{
			a = m_iLensGridSegmentCountH*y + x + offset;
			b = m_iLensGridSegmentCountH*y + x + 1 + offset;
			c = (y + 1)*m_iLensGridSegmentCountH + x + 1 + offset;
			d = (y + 1)*m_iLensGridSegmentCountH + x + offset;
			vIndices.push_back( a );
			vIndices.push_back( b );
			vIndices.push_back( c );

			vIndices.push_back( a );
			vIndices.push_back( c );
			vIndices.push_back( d );
		}
	}

	offset = (m_iLensGridSegmentCountH)*(m_iLensGridSegmentCountV);
	for( GLushort y = 0; y<m_iLensGridSegmentCountV - 1; y++ )
	{
		for( GLushort x = 0; x<m_iLensGridSegmentCountH - 1; x++ )
		{
			a = m_iLensGridSegmentCountH*y + x + offset;
			b = m_iLensGridSegmentCountH*y + x + 1 + offset;
			c = (y + 1)*m_iLensGridSegmentCountH + x + 1 + offset;
			d = (y + 1)*m_iLensGridSegmentCountH + x + offset;
			vIndices.push_back( a );
			vIndices.push_back( b );
			vIndices.push_back( c );

			vIndices.push_back( a );
			vIndices.push_back( c );
			vIndices.push_back( d );
		}
	}
	m_uiIndexSize = vIndices.size();

	glGenVertexArrays( 1, &m_unLensVAO );
	glBindVertexArray( m_unLensVAO );

	glGenBuffers( 1, &m_glIDVertBuffer );
	glBindBuffer( GL_ARRAY_BUFFER, m_glIDVertBuffer );
	glBufferData( GL_ARRAY_BUFFER, vVerts.size()*sizeof( VertexDataLens ), &vVerts[0], GL_STATIC_DRAW );

	glGenBuffers( 1, &m_glIDIndexBuffer );
	glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, m_glIDIndexBuffer );
	glBufferData( GL_ELEMENT_ARRAY_BUFFER, vIndices.size()*sizeof( GLushort ), &vIndices[0], GL_STATIC_DRAW );

	glEnableVertexAttribArray( 0 );
	glVertexAttribPointer( 0, 2, GL_FLOAT, GL_FALSE, sizeof( VertexDataLens ), (void *)offsetof( VertexDataLens, position ) );

	glEnableVertexAttribArray( 1 );
	glVertexAttribPointer( 1, 2, GL_FLOAT, GL_FALSE, sizeof( VertexDataLens ), (void *)offsetof( VertexDataLens, texCoordRed ) );

	glEnableVertexAttribArray( 2 );
	glVertexAttribPointer( 2, 2, GL_FLOAT, GL_FALSE, sizeof( VertexDataLens ), (void *)offsetof( VertexDataLens, texCoordGreen ) );

	glEnableVertexAttribArray( 3 );
	glVertexAttribPointer( 3, 2, GL_FLOAT, GL_FALSE, sizeof( VertexDataLens ), (void *)offsetof( VertexDataLens, texCoordBlue ) );

	glBindVertexArray( 0 );

	glDisableVertexAttribArray( 0 );
	glDisableVertexAttribArray( 1 );
	glDisableVertexAttribArray( 2 );
	glDisableVertexAttribArray( 3 );

	glBindBuffer( GL_ARRAY_BUFFER, 0 );
	glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0 );
}

void HelloVrApp::RenderStereoTargets()
{
	glClearColor( 0.15f, 0.15f, 0.18f, 1.0f ); // nice background color, but not black
	glEnable( GL_MULTISAMPLE );

	// Left Eye
	glBindFramebuffer( GL_FRAMEBUFFER, leftEyeDesc.m_nRenderFramebufferId );
	glViewport( 0, 0, m_nRenderWidth, m_nRenderHeight );
	RenderScene( vr::Eye_Left );
	glBindFramebuffer( GL_FRAMEBUFFER, 0 );

	glDisable( GL_MULTISAMPLE );

	glBindFramebuffer( GL_READ_FRAMEBUFFER, leftEyeDesc.m_nRenderFramebufferId );
	glBindFramebuffer( GL_DRAW_FRAMEBUFFER, leftEyeDesc.m_nResolveFramebufferId );

	glBlitFramebuffer( 0, 0, m_nRenderWidth, m_nRenderHeight, 0, 0, m_nRenderWidth, m_nRenderHeight,
		GL_COLOR_BUFFER_BIT,
		GL_LINEAR );

	glBindFramebuffer( GL_READ_FRAMEBUFFER, 0 );
	glBindFramebuffer( GL_DRAW_FRAMEBUFFER, 0 );

	glEnable( GL_MULTISAMPLE );

	// Right Eye
	glBindFramebuffer( GL_FRAMEBUFFER, rightEyeDesc.m_nRenderFramebufferId );
	glViewport( 0, 0, m_nRenderWidth, m_nRenderHeight );
	RenderScene( vr::Eye_Right );
	glBindFramebuffer( GL_FRAMEBUFFER, 0 );

	glDisable( GL_MULTISAMPLE );

	glBindFramebuffer( GL_READ_FRAMEBUFFER, rightEyeDesc.m_nRenderFramebufferId );
	glBindFramebuffer( GL_DRAW_FRAMEBUFFER, rightEyeDesc.m_nResolveFramebufferId );

	glBlitFramebuffer( 0, 0, m_nRenderWidth, m_nRenderHeight, 0, 0, m_nRenderWidth, m_nRenderHeight,
		GL_COLOR_BUFFER_BIT,
		GL_LINEAR );

	glBindFramebuffer( GL_READ_FRAMEBUFFER, 0 );
	glBindFramebuffer( GL_DRAW_FRAMEBUFFER, 0 );
}

void HelloVrApp::RenderScene( vr::Hmd_Eye nEye )
{
	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
	glEnable( GL_DEPTH_TEST );

	if( m_bShowCubes )
	{
		glUseProgram( m_unSceneProgramID );
		glUniformMatrix4fv( m_nSceneMatrixLocation, 1, GL_FALSE, glm::value_ptr( GetCurrentViewProjectionMatrix( nEye ) ) );
		glBindVertexArray( m_unSceneVAO );
		glBindTexture( GL_TEXTURE_2D, mCubeTexture->getId() );
		glDrawArrays( GL_TRIANGLES, 0, m_uiVertcount );
		glBindVertexArray( 0 );
	}

	bool bIsInputCapturedByAnotherProcess = m_pHMD->IsInputFocusCapturedByAnotherProcess();

	if( !bIsInputCapturedByAnotherProcess )
	{
		// draw the controller axis lines
		glUseProgram( m_unControllerTransformProgramID );
		glUniformMatrix4fv( m_nControllerMatrixLocation, 1, GL_FALSE, glm::value_ptr( GetCurrentViewProjectionMatrix( nEye ) ) );
		glBindVertexArray( m_unControllerVAO );
		glDrawArrays( GL_LINES, 0, m_uiControllerVertcount );
		glBindVertexArray( 0 );
	}

	// ----- Render Model rendering -----
	glUseProgram( m_unRenderModelProgramID );

	for( uint32_t unTrackedDevice = 0; unTrackedDevice < vr::k_unMaxTrackedDeviceCount; unTrackedDevice++ )
	{
		if( !m_rTrackedDeviceToRenderModel[unTrackedDevice] || !m_rbShowTrackedDevice[unTrackedDevice] )
			continue;

		const vr::TrackedDevicePose_t & pose = m_rTrackedDevicePose[unTrackedDevice];
		if( !pose.bPoseIsValid )
			continue;

		if( bIsInputCapturedByAnotherProcess && m_pHMD->GetTrackedDeviceClass( unTrackedDevice ) == vr::TrackedDeviceClass_Controller )
			continue;

		const glm::mat4 & matDeviceToTracking = m_rmat4DevicePose[unTrackedDevice];
		glm::mat4 matMVP = GetCurrentViewProjectionMatrix( nEye ) * matDeviceToTracking;
		glUniformMatrix4fv( m_nRenderModelMatrixLocation, 1, GL_FALSE, glm::value_ptr( matMVP ) );

		m_rTrackedDeviceToRenderModel[unTrackedDevice]->Draw();
	}

	glUseProgram( 0 );
}

void HelloVrApp::RenderDistortion()
{
	glDisable( GL_DEPTH_TEST );
	glViewport( 0, 0, m_nWindowWidth, m_nWindowHeight );

	glBindVertexArray( m_unLensVAO );
	glUseProgram( m_unLensProgramID );

	//render left lens (first half of index array )
	glBindTexture( GL_TEXTURE_2D, leftEyeDesc.m_nResolveTextureId );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );
	glDrawElements( GL_TRIANGLES, m_uiIndexSize / 2, GL_UNSIGNED_SHORT, 0 );

	//render right lens (second half of index array )
	glBindTexture( GL_TEXTURE_2D, rightEyeDesc.m_nResolveTextureId );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );
	glDrawElements( GL_TRIANGLES, m_uiIndexSize / 2, GL_UNSIGNED_SHORT, (const void *)(m_uiIndexSize) );

	glBindVertexArray( 0 );
	glUseProgram( 0 );
}

glm::mat4 HelloVrApp::GetHMDMatrixProjectionEye( vr::Hmd_Eye nEye )
{
	if( !m_pHMD )
		return glm::mat4();

	auto mat = m_pHMD->GetProjectionMatrix( nEye, m_fNearClip, m_fFarClip, vr::API_OpenGL );

	return glm::mat4(
		mat.m[0][0], mat.m[1][0], mat.m[2][0], mat.m[3][0],
		mat.m[0][1], mat.m[1][1], mat.m[2][1], mat.m[3][1],
		mat.m[0][2], mat.m[1][2], mat.m[2][2], mat.m[3][2],
		mat.m[0][3], mat.m[1][3], mat.m[2][3], mat.m[3][3]
		);
}

glm::mat4 HelloVrApp::GetHMDMatrixPoseEye( vr::Hmd_Eye nEye )
{
	if( !m_pHMD )
		return glm::mat4();

	vr::HmdMatrix34_t matEyeRight = m_pHMD->GetEyeToHeadTransform( nEye );
	glm::mat4 matrixObj(
		matEyeRight.m[0][0], matEyeRight.m[1][0], matEyeRight.m[2][0], 0.0,
		matEyeRight.m[0][1], matEyeRight.m[1][1], matEyeRight.m[2][1], 0.0,
		matEyeRight.m[0][2], matEyeRight.m[1][2], matEyeRight.m[2][2], 0.0,
		matEyeRight.m[0][3], matEyeRight.m[1][3], matEyeRight.m[2][3], 1.0f
		);

	return glm::inverse( matrixObj );
}

glm::mat4 HelloVrApp::GetCurrentViewProjectionMatrix( vr::Hmd_Eye nEye )
{
	glm::mat4 matMVP;
	if( nEye == vr::Eye_Left )
	{
		matMVP = m_mat4ProjectionLeft * m_mat4eyePosLeft * m_mat4HMDPose;
	}
	else if( nEye == vr::Eye_Right )
	{
		matMVP = m_mat4ProjectionRight * m_mat4eyePosRight *  m_mat4HMDPose;
	}

	return matMVP;
}

void HelloVrApp::UpdateHMDMatrixPose()
{
	if( !m_pHMD )
		return;

	vr::VRCompositor()->WaitGetPoses( m_rTrackedDevicePose, vr::k_unMaxTrackedDeviceCount, NULL, 0 );

	m_iValidPoseCount = 0;
	m_strPoseClasses = "";
	for( int nDevice = 0; nDevice < vr::k_unMaxTrackedDeviceCount; ++nDevice )
	{
		if( m_rTrackedDevicePose[nDevice].bPoseIsValid )
		{
			m_iValidPoseCount++;
			m_rmat4DevicePose[nDevice] = ConvertSteamVRMatrixToMat4( m_rTrackedDevicePose[nDevice].mDeviceToAbsoluteTracking );
			if( m_rDevClassChar[nDevice] == 0 )
			{
				switch( m_pHMD->GetTrackedDeviceClass( nDevice ) )
				{
				case vr::TrackedDeviceClass_Controller:        m_rDevClassChar[nDevice] = 'C'; break;
				case vr::TrackedDeviceClass_HMD:               m_rDevClassChar[nDevice] = 'H'; break;
				case vr::TrackedDeviceClass_Invalid:           m_rDevClassChar[nDevice] = 'I'; break;
				case vr::TrackedDeviceClass_Other:             m_rDevClassChar[nDevice] = 'O'; break;
				case vr::TrackedDeviceClass_TrackingReference: m_rDevClassChar[nDevice] = 'T'; break;
				default:                                       m_rDevClassChar[nDevice] = '?'; break;
				}
			}
			m_strPoseClasses += m_rDevClassChar[nDevice];
		}
	}

	if( m_rTrackedDevicePose[vr::k_unTrackedDeviceIndex_Hmd].bPoseIsValid )
	{
		m_mat4HMDPose = glm::inverse( m_rmat4DevicePose[vr::k_unTrackedDeviceIndex_Hmd] );
	}
}

CGLRenderModel *HelloVrApp::FindOrLoadRenderModel( const char *pchRenderModelName )
{
	CGLRenderModel *pRenderModel = NULL;
	for( std::vector< CGLRenderModel * >::iterator i = m_vecRenderModels.begin(); i != m_vecRenderModels.end(); i++ )
	{
		if( !stricmp( (*i)->GetName().c_str(), pchRenderModelName ) )
		{
			pRenderModel = *i;
			break;
		}
	}

	// load the model if we didn't find one
	if( !pRenderModel )
	{
		vr::RenderModel_t *pModel = NULL;
		if( !vr::VRRenderModels()->LoadRenderModel( pchRenderModelName, &pModel ) || pModel == NULL )
		{
			CI_LOG_E( "Unable to load render model %s\n", pchRenderModelName );
			return NULL; // move on to the next tracked device
		}

		vr::RenderModel_TextureMap_t *pTexture = NULL;
		if( !vr::VRRenderModels()->LoadTexture( pModel->diffuseTextureId, &pTexture ) || pTexture == NULL )
		{
			CI_LOG_E( "Unable to load render texture id:%d for render model %s\n", pModel->diffuseTextureId, pchRenderModelName );
			vr::VRRenderModels()->FreeRenderModel( pModel );
			return NULL; // move on to the next tracked device
		}

		pRenderModel = new CGLRenderModel( pchRenderModelName );
		if( !pRenderModel->BInit( *pModel, *pTexture ) )
		{
			CI_LOG_E( "Unable to create GL model from render model %s\n", pchRenderModelName );
			delete pRenderModel;
			pRenderModel = NULL;
		}
		else
		{
			m_vecRenderModels.push_back( pRenderModel );
		}
		vr::VRRenderModels()->FreeRenderModel( pModel );
		vr::VRRenderModels()->FreeTexture( pTexture );
	}
	return pRenderModel;
}


//-----------------------------------------------------------------------------
// Purpose: Create/destroy GL a Render Model for a single tracked device
//-----------------------------------------------------------------------------
void HelloVrApp::SetupRenderModelForTrackedDevice( vr::TrackedDeviceIndex_t unTrackedDeviceIndex )
{
	if( unTrackedDeviceIndex >= vr::k_unMaxTrackedDeviceCount )
		return;

	// try to find a model we've already set up
	std::string sRenderModelName = GetTrackedDeviceString( m_pHMD, unTrackedDeviceIndex, vr::Prop_RenderModelName_String );
	CGLRenderModel *pRenderModel = FindOrLoadRenderModel( sRenderModelName.c_str() );
	if( !pRenderModel )
	{
		std::string sTrackingSystemName = GetTrackedDeviceString( m_pHMD, unTrackedDeviceIndex, vr::Prop_TrackingSystemName_String );
		CI_LOG_E( "Unable to load render model for tracked device %d (%s.%s)", unTrackedDeviceIndex, sTrackingSystemName.c_str(), sRenderModelName.c_str() );
	}
	else
	{
		m_rTrackedDeviceToRenderModel[unTrackedDeviceIndex] = pRenderModel;
		m_rbShowTrackedDevice[unTrackedDeviceIndex] = true;
	}
}


//-----------------------------------------------------------------------------
// Purpose: Create/destroy GL Render Models
//-----------------------------------------------------------------------------
void HelloVrApp::SetupRenderModels()
{
	memset( m_rTrackedDeviceToRenderModel, 0, sizeof( m_rTrackedDeviceToRenderModel ) );

	if( !m_pHMD )
		return;

	for( uint32_t unTrackedDevice = vr::k_unTrackedDeviceIndex_Hmd + 1; unTrackedDevice < vr::k_unMaxTrackedDeviceCount; unTrackedDevice++ )
	{
		if( !m_pHMD->IsTrackedDeviceConnected( unTrackedDevice ) )
			continue;

		SetupRenderModelForTrackedDevice( unTrackedDevice );
	}

}


//-----------------------------------------------------------------------------
// Purpose: Converts a SteamVR matrix to our local matrix class
//-----------------------------------------------------------------------------
glm::mat4 HelloVrApp::ConvertSteamVRMatrixToMat4( const vr::HmdMatrix34_t &matPose )
{
	glm::mat4 matrixObj(
		matPose.m[0][0], matPose.m[1][0], matPose.m[2][0], 0.0,
		matPose.m[0][1], matPose.m[1][1], matPose.m[2][1], 0.0,
		matPose.m[0][2], matPose.m[1][2], matPose.m[2][2], 0.0,
		matPose.m[0][3], matPose.m[1][3], matPose.m[2][3], 1.0f
		);
	return matrixObj;
}


//-----------------------------------------------------------------------------
// Purpose: Create/destroy GL Render Models
//-----------------------------------------------------------------------------
CGLRenderModel::CGLRenderModel( const std::string & sRenderModelName )
	: m_sModelName( sRenderModelName )
{
	m_glIndexBuffer = 0;
	m_glVertArray = 0;
	m_glVertBuffer = 0;
	m_glTexture = 0;
}


CGLRenderModel::~CGLRenderModel()
{
	Cleanup();
}


//-----------------------------------------------------------------------------
// Purpose: Allocates and populates the GL resources for a render model
//-----------------------------------------------------------------------------
bool CGLRenderModel::BInit( const vr::RenderModel_t & vrModel, const vr::RenderModel_TextureMap_t & vrDiffuseTexture )
{
	// create and bind a VAO to hold state for this model
	glGenVertexArrays( 1, &m_glVertArray );
	glBindVertexArray( m_glVertArray );

	// Populate a vertex buffer
	glGenBuffers( 1, &m_glVertBuffer );
	glBindBuffer( GL_ARRAY_BUFFER, m_glVertBuffer );
	glBufferData( GL_ARRAY_BUFFER, sizeof( vr::RenderModel_Vertex_t ) * vrModel.unVertexCount, vrModel.rVertexData, GL_STATIC_DRAW );

	// Identify the components in the vertex buffer
	glEnableVertexAttribArray( 0 );
	glVertexAttribPointer( 0, 3, GL_FLOAT, GL_FALSE, sizeof( vr::RenderModel_Vertex_t ), (void *)offsetof( vr::RenderModel_Vertex_t, vPosition ) );
	glEnableVertexAttribArray( 1 );
	glVertexAttribPointer( 1, 3, GL_FLOAT, GL_FALSE, sizeof( vr::RenderModel_Vertex_t ), (void *)offsetof( vr::RenderModel_Vertex_t, vNormal ) );
	glEnableVertexAttribArray( 2 );
	glVertexAttribPointer( 2, 2, GL_FLOAT, GL_FALSE, sizeof( vr::RenderModel_Vertex_t ), (void *)offsetof( vr::RenderModel_Vertex_t, rfTextureCoord ) );

	// Create and populate the index buffer
	glGenBuffers( 1, &m_glIndexBuffer );
	glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, m_glIndexBuffer );
	glBufferData( GL_ELEMENT_ARRAY_BUFFER, sizeof( uint16_t ) * vrModel.unTriangleCount * 3, vrModel.rIndexData, GL_STATIC_DRAW );

	glBindVertexArray( 0 );

	// create and populate the texture
	glGenTextures( 1, &m_glTexture );
	glBindTexture( GL_TEXTURE_2D, m_glTexture );

	glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, vrDiffuseTexture.unWidth, vrDiffuseTexture.unHeight,
		0, GL_RGBA, GL_UNSIGNED_BYTE, vrDiffuseTexture.rubTextureMapData );

	// If this renders black ask McJohn what's wrong.
	glGenerateMipmap( GL_TEXTURE_2D );

	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );

	GLfloat fLargest;
	glGetFloatv( GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &fLargest );
	glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, fLargest );

	glBindTexture( GL_TEXTURE_2D, 0 );

	m_unVertexCount = vrModel.unTriangleCount * 3;

	return true;
}


//-----------------------------------------------------------------------------
// Purpose: Frees the GL resources for a render model
//-----------------------------------------------------------------------------
void CGLRenderModel::Cleanup()
{
	if( m_glVertBuffer )
	{
		glDeleteBuffers( 1, &m_glIndexBuffer );
		glDeleteBuffers( 1, &m_glVertArray );
		glDeleteBuffers( 1, &m_glVertBuffer );
		m_glIndexBuffer = 0;
		m_glVertArray = 0;
		m_glVertBuffer = 0;
	}
}


//-----------------------------------------------------------------------------
// Purpose: Draws the render model
//-----------------------------------------------------------------------------
void CGLRenderModel::Draw()
{
	glBindVertexArray( m_glVertArray );

	glActiveTexture( GL_TEXTURE0 );
	glBindTexture( GL_TEXTURE_2D, m_glTexture );

	glDrawElements( GL_TRIANGLES, m_unVertexCount, GL_UNSIGNED_SHORT, 0 );

	glBindVertexArray( 0 );
}

void HelloVrApp::mouseDown( MouseEvent event )
{
}

void HelloVrApp::update()
{
	// Process SteamVR events
	vr::VREvent_t event;
	while( m_pHMD->PollNextEvent( &event ) )
	{
		ProcessVREvent( event );
	}

	// Process SteamVR controller state
	for( vr::TrackedDeviceIndex_t unDevice = 0; unDevice < vr::k_unMaxTrackedDeviceCount; unDevice++ )
	{
		vr::VRControllerState_t state;
		if( m_pHMD->GetControllerState( unDevice, &state ) )
		{
			m_rbShowTrackedDevice[unDevice] = state.ulButtonPressed == 0;
		}
	}
}

void HelloVrApp::draw()
{
	UpdateHMDMatrixPose();
	
	// for now as fast as possible
	if( m_pHMD )
	{
		DrawControllers();
		RenderStereoTargets();
		RenderDistortion();

		vr::Texture_t leftEyeTexture = { (void*)leftEyeDesc.m_nResolveTextureId, vr::API_OpenGL, vr::ColorSpace_Gamma };
		vr::VRCompositor()->Submit( vr::Eye_Left, &leftEyeTexture );
		vr::Texture_t rightEyeTexture = { (void*)rightEyeDesc.m_nResolveTextureId, vr::API_OpenGL, vr::ColorSpace_Gamma };
		vr::VRCompositor()->Submit( vr::Eye_Right, &rightEyeTexture );
	}

	//if( m_bVblank && m_bGlFinishHack )
	//{
	//	//$ HACKHACK. From gpuview profiling, it looks like there is a bug where two renders and a present
	//	// happen right before and after the vsync causing all kinds of jittering issues. This glFinish()
	//	// appears to clear that up. Temporary fix while I try to get nvidia to investigate this problem.
	//	// 1/29/2014 mikesart
	//	glFinish();
	//}

	//// SwapWindow
	//{
	//	SDL_GL_SwapWindow( m_pWindow );
	//}

	//// Clear
	//{
	//	// We want to make sure the glFinish waits for the entire present to complete, not just the submission
	//	// of the command. So, we do a clear here right here so the glFinish will wait fully for the swap.
	//	glClearColor( 0, 0, 0, 1 );
	//	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
	//}

	//// Flush and wait for swap.
	//if( m_bVblank )
	//{
	//	glFlush();
	//	glFinish();
	//}

	// Spew out the controller and pose count whenever they change.
	if( m_iTrackedControllerCount != m_iTrackedControllerCount_Last || m_iValidPoseCount != m_iValidPoseCount_Last )
	{
		m_iValidPoseCount_Last = m_iValidPoseCount;
		m_iTrackedControllerCount_Last = m_iTrackedControllerCount;

		//CI_LOG_I( "PoseCount:%d(%s) Controllers:%d\n", m_iValidPoseCount, m_strPoseClasses.c_str(), m_iTrackedControllerCount );
	}
}

void HelloVrApp::cleanup()
{
	//if( m_pHMD )
	//{
	//	//vr::VR_Shutdown();
	//	m_pHMD = NULL;
	//}

	for( std::vector< CGLRenderModel * >::iterator i = m_vecRenderModels.begin(); i != m_vecRenderModels.end(); i++ )
	{
		delete (*i);
	}
	m_vecRenderModels.clear();

	if( gl::context() )
	{
		glDebugMessageControl( GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_FALSE );
		glDebugMessageCallback( nullptr, nullptr );
		glDeleteBuffers( 1, &m_glSceneVertBuffer );
		glDeleteBuffers( 1, &m_glIDVertBuffer );
		glDeleteBuffers( 1, &m_glIDIndexBuffer );

		if( m_unSceneProgramID )
		{
			glDeleteProgram( m_unSceneProgramID );
		}
		if( m_unControllerTransformProgramID )
		{
			glDeleteProgram( m_unControllerTransformProgramID );
		}
		if( m_unRenderModelProgramID )
		{
			glDeleteProgram( m_unRenderModelProgramID );
		}
		if( m_unLensProgramID )
		{
			glDeleteProgram( m_unLensProgramID );
		}

		glDeleteRenderbuffers( 1, &leftEyeDesc.m_nDepthBufferId );
		glDeleteTextures( 1, &leftEyeDesc.m_nRenderTextureId );
		glDeleteFramebuffers( 1, &leftEyeDesc.m_nRenderFramebufferId );
		glDeleteTextures( 1, &leftEyeDesc.m_nResolveTextureId );
		glDeleteFramebuffers( 1, &leftEyeDesc.m_nResolveFramebufferId );

		glDeleteRenderbuffers( 1, &rightEyeDesc.m_nDepthBufferId );
		glDeleteTextures( 1, &rightEyeDesc.m_nRenderTextureId );
		glDeleteFramebuffers( 1, &rightEyeDesc.m_nRenderFramebufferId );
		glDeleteTextures( 1, &rightEyeDesc.m_nResolveTextureId );
		glDeleteFramebuffers( 1, &rightEyeDesc.m_nResolveFramebufferId );

		if( m_unLensVAO != 0 )
		{
			glDeleteVertexArrays( 1, &m_unLensVAO );
		}
		if( m_unSceneVAO != 0 )
		{
			glDeleteVertexArrays( 1, &m_unSceneVAO );
		}
		if( m_unControllerVAO != 0 )
		{
			glDeleteVertexArrays( 1, &m_unControllerVAO );
		}
	}
}

void HelloVrApp::keyDown( KeyEvent event )
{
	if( event.getCode() == KeyEvent::KEY_ESCAPE ) {
		quit();
	}
}

void prepareSettings( App::Settings* settings )
{
	settings->setWindowSize( 1280, 720 );
	settings->disableFrameRate();
}

CINDER_APP( HelloVrApp, RendererGl( RendererGl::Options().msaa( 16 ) ), prepareSettings )