#include "LBuffer.h"

LBuffer::LBuffer(GBuffer* GBufferIn_ptr)
{
    m_GBuffer_ptr = GBufferIn_ptr;
    m_LBufferData.screenWidth = m_GBuffer_ptr->GetBufferWidth();
    m_LBufferData.screenHeight = m_GBuffer_ptr->GetBufferHeight();

    m_LBufferData.bufferId = rlLoadFramebuffer(m_LBufferData.screenWidth, m_LBufferData.screenHeight);   // Load an empty framebuffer

    if (m_LBufferData.bufferId > 0)
    {
        rlEnableFramebuffer(m_LBufferData.bufferId);
	   
        // Create HDR direct lighting tex 32bit RGBA 
        m_LBufferData.DirectLighting.id = rlLoadTexture(NULL, m_LBufferData.screenWidth, m_LBufferData.screenHeight, PIXELFORMAT_UNCOMPRESSED_R32G32B32A32, 1);
        m_LBufferData.DirectLighting.width = m_LBufferData.screenWidth;
        m_LBufferData.DirectLighting.height = m_LBufferData.screenHeight;
        m_LBufferData.DirectLighting.format = PIXELFORMAT_UNCOMPRESSED_R32G32B32A32;
        m_LBufferData.DirectLighting.mipmaps = 1;

        // Create HDR indirect lighting tex 32bit RGBA
        m_LBufferData.IndirectLighting.id = rlLoadTexture(NULL, m_LBufferData.screenWidth, m_LBufferData.screenHeight, PIXELFORMAT_UNCOMPRESSED_R32G32B32A32, 1);
        m_LBufferData.IndirectLighting.width = m_LBufferData.screenWidth;
        m_LBufferData.IndirectLighting.height = m_LBufferData.screenHeight;
        m_LBufferData.IndirectLighting.format = PIXELFORMAT_UNCOMPRESSED_R32G32B32A32;
        m_LBufferData.IndirectLighting.mipmaps = 1;

        // Create HDR final light tex 32bit RGBA
        m_LBufferData.FinalIlluminatedScene.id = rlLoadTexture(NULL, m_LBufferData.screenWidth, m_LBufferData.screenHeight, PIXELFORMAT_UNCOMPRESSED_R32G32B32A32, 1);
        m_LBufferData.FinalIlluminatedScene.width = m_LBufferData.screenWidth;
        m_LBufferData.FinalIlluminatedScene.height = m_LBufferData.screenHeight;
        m_LBufferData.FinalIlluminatedScene.format = PIXELFORMAT_UNCOMPRESSED_R32G32B32A32;
        m_LBufferData.FinalIlluminatedScene.mipmaps = 1;

        // Create Depth tex 32bit single channel
        m_LBufferData.Depth.id = rlLoadTextureDepth(m_LBufferData.screenWidth, m_LBufferData.screenHeight, false);
        m_LBufferData.Depth.width = m_LBufferData.screenWidth;
        m_LBufferData.Depth.height = m_LBufferData.screenHeight;
        m_LBufferData.Depth.format = PIXELFORMAT_UNCOMPRESSED_R32;
        m_LBufferData.Depth.mipmaps = 1;

        // Attach all textures to FBO
        rlFramebufferAttach(m_LBufferData.bufferId, m_LBufferData.DirectLighting.id, RL_ATTACHMENT_COLOR_CHANNEL0, RL_ATTACHMENT_TEXTURE2D, 0);
        rlFramebufferAttach(m_LBufferData.bufferId, m_LBufferData.IndirectLighting.id, RL_ATTACHMENT_COLOR_CHANNEL1, RL_ATTACHMENT_TEXTURE2D, 0);
        rlFramebufferAttach(m_LBufferData.bufferId, m_LBufferData.FinalIlluminatedScene.id, RL_ATTACHMENT_COLOR_CHANNEL2, RL_ATTACHMENT_TEXTURE2D, 0);

        rlFramebufferAttach(m_LBufferData.bufferId, m_LBufferData.Depth.id, RL_ATTACHMENT_DEPTH, RL_ATTACHMENT_DEPTH, 0);

        // Check if fbo is complete with attachments (valid)
        if (rlFramebufferComplete(m_LBufferData.bufferId)) TRACELOG(LOG_INFO, "FBO: [ID %i] Framebuffer object created successfully", m_LBufferData.bufferId);

        rlEnableFramebuffer(m_LBufferData.bufferId);

        rlActiveDrawBuffers(3);

        rlDisableFramebuffer();
    }
    else TRACELOG(LOG_WARNING, "FBO: Framebuffer object can not be created");

    // Init shaders, mats, and models
    m_DirectionalLightShader.shader = LoadShader("D:/LBufferShader.vs", "D:/LBufferDirectionalShader.fs");
    m_DirectionalLightShader.shader.locs[SHADER_LOC_MAP_OCCLUSION] = rlGetLocationUniform(m_DirectionalLightShader.shader.id, "WorldPosTex"); //no default worldpos map so we use an empty non-cubemap
    m_DirectionalLightShader.shader.locs[SHADER_LOC_MAP_NORMAL] = rlGetLocationUniform(m_DirectionalLightShader.shader.id, "NormalTex");
    m_DirectionalLightShader.shader.locs[SHADER_LOC_MAP_ALBEDO] = rlGetLocationUniform(m_DirectionalLightShader.shader.id, "AlbedoTex");
    m_DirectionalLightShader.shader.locs[SHADER_LOC_MAP_METALNESS] = rlGetLocationUniform(m_DirectionalLightShader.shader.id, "MetalnessTex");
    m_DirectionalLightShader.shader.locs[SHADER_LOC_MAP_ROUGHNESS] = rlGetLocationUniform(m_DirectionalLightShader.shader.id, "RoughnessTex");
    m_DirectionalLightShader.shader.locs[SHADER_LOC_MAP_EMISSION] = rlGetLocationUniform(m_DirectionalLightShader.shader.id, "EmissiveTex"); //Emissive light is added in the directional light pass
    m_DirectionalLightShader.shader.locs[SHADER_LOC_MAP_BRDF] = rlGetLocationUniform(m_DirectionalLightShader.shader.id, "DirectLightPassTex"); //once again using an empty tex map location
    m_DirectionalLightShader.cameraPositionLoc = GetShaderLocation(m_DirectionalLightShader.shader, "CameraWorldPos");
    m_DirectionalLightShader.colorLoc = GetShaderLocation(m_DirectionalLightShader.shader, "SampledDirectionalLight.Base.color");
    m_DirectionalLightShader.ambientIntensityLoc = GetShaderLocation(m_DirectionalLightShader.shader, "SampledDirectionalLight.Base.ambientIntensity");
    m_DirectionalLightShader.diffuseIntensityLoc = GetShaderLocation(m_DirectionalLightShader.shader, "SampledDirectionalLight.Base.diffuseIntensity");
    m_DirectionalLightShader.directionLoc = GetShaderLocation(m_DirectionalLightShader.shader, "SampledDirectionalLight.direction");

    //m_PointLightShader.shader = LoadShader(0, "D:/LBufferPointShader.fs");
    //m_PointLightShader.shader.locs[SHADER_LOC_MAP_OCCLUSION] = GetShaderLocation(m_PointLightShader.shader, "WorldPosTex"); //no default worldpos map so we use an empty non-cubemap
    //m_PointLightShader.shader.locs[SHADER_LOC_MAP_NORMAL] = GetShaderLocation(m_PointLightShader.shader, "NormalTex");
    //m_PointLightShader.shader.locs[SHADER_LOC_MAP_ALBEDO] = GetShaderLocation(m_PointLightShader.shader, "AlbedoTex");
    //m_PointLightShader.shader.locs[SHADER_LOC_MAP_METALNESS] = GetShaderLocation(m_PointLightShader.shader, "MetalnessTex");
    //m_PointLightShader.shader.locs[SHADER_LOC_MAP_ROUGHNESS] = GetShaderLocation(m_PointLightShader.shader, "RoughnessTex");
    //m_PointLightShader.shader.locs[SHADER_LOC_MAP_BRDF] = GetShaderLocation(m_PointLightShader.shader, "DirectLightPassTex"); //once again using an empty tex map location
    //m_PointLightShader.cameraPositionLoc = GetShaderLocation(m_PointLightShader.shader, "CameraWorldPos");
    //m_PointLightShader.colorLoc = GetShaderLocation(m_PointLightShader.shader, "PointLight.Base.color");
    //m_PointLightShader.ambientIntensityLoc = GetShaderLocation(m_PointLightShader.shader, "PointLight.Base.ambientIntensity");
    //m_PointLightShader.diffuseIntensityLoc = GetShaderLocation(m_PointLightShader.shader, "PointLight.Base.diffuseIntensity");
    //m_PointLightShader.positionLoc = GetShaderLocation(m_PointLightShader.shader, "PointLight.position");
    //m_PointLightShader.constantAttLoc = GetShaderLocation(m_PointLightShader.shader, "PointLight.constantAtt");
    //m_PointLightShader.linearAttLoc = GetShaderLocation(m_PointLightShader.shader, "PointLight.linearAtt");
    //m_PointLightShader.exponentialAttLoc = GetShaderLocation(m_PointLightShader.shader, "PointLight.exponentialAtt");

    //m_SpotLightShader.shader = LoadShader(0, "D:/LBufferSpotShader.fs");
    //m_SpotLightShader.shader.locs[SHADER_LOC_MAP_OCCLUSION] = GetShaderLocation(m_SpotLightShader.shader, "WorldPosTex"); //no default worldpos map so we use an empty non-cubemap
    //m_SpotLightShader.shader.locs[SHADER_LOC_MAP_NORMAL] = GetShaderLocation(m_SpotLightShader.shader, "NormalTex");
    //m_SpotLightShader.shader.locs[SHADER_LOC_MAP_ALBEDO] = GetShaderLocation(m_SpotLightShader.shader, "AlbedoTex");
    //m_SpotLightShader.shader.locs[SHADER_LOC_MAP_METALNESS] = GetShaderLocation(m_SpotLightShader.shader, "MetalnessTex");
    //m_SpotLightShader.shader.locs[SHADER_LOC_MAP_ROUGHNESS] = GetShaderLocation(m_SpotLightShader.shader, "RoughnessTex");
    //m_SpotLightShader.shader.locs[SHADER_LOC_MAP_BRDF] = GetShaderLocation(m_SpotLightShader.shader, "DirectLightPassTex"); //once again using an empty tex map location
    //m_SpotLightShader.cameraPositionLoc = GetShaderLocation(m_SpotLightShader.shader, "CameraWorldPos");
    //m_SpotLightShader.colorLoc = GetShaderLocation(m_SpotLightShader.shader, "SpotLight.Base.Base.color");
    //m_SpotLightShader.ambientIntensityLoc = GetShaderLocation(m_SpotLightShader.shader, "SpotLight.Base.Base.ambientIntensity");
    //m_SpotLightShader.diffuseIntensityLoc = GetShaderLocation(m_SpotLightShader.shader, "SpotLight.Base.Base.diffuseIntensity");
    //m_SpotLightShader.positionLoc = GetShaderLocation(m_SpotLightShader.shader, "SpotLight.Base.position");
    //m_SpotLightShader.constantAttLoc = GetShaderLocation(m_SpotLightShader.shader, "SpotLight.Base.constantAtt");
    //m_SpotLightShader.linearAttLoc = GetShaderLocation(m_SpotLightShader.shader, "SpotLight.Base.linearAtt");
    //m_SpotLightShader.exponentialAttLoc = GetShaderLocation(m_SpotLightShader.shader, "SpotLight.Base.exponentialAtt");
    //m_SpotLightShader.directionLoc = GetShaderLocation(m_SpotLightShader.shader, "SpotLight.direction");
    //m_SpotLightShader.cutOffAngleLoc = GetShaderLocation(m_SpotLightShader.shader, "SpotLight.cutoff");


   // m_pointlLightShaderMat.shader = m_PointLightShader.shader;
    //m_spotLightShaderMat.shader = m_SpotLightShader.shader;

    DirectionalLight testLight;
    m_DirectionalLightContainer.push_back(testLight);


    float screenRatio = float(m_LBufferData.screenWidth) / float(m_LBufferData.screenHeight);
    Mesh tempMesh = GenMeshPlane(screenRatio, 1.0f, 1, 1);
    m_planeModel = LoadModelFromMesh(tempMesh);
    Material tempMat;
    tempMat.shader = m_DirectionalLightShader.shader;

    m_planeModel.materials[0].maps[MATERIAL_MAP_ALBEDO].texture = m_GBuffer_ptr->GetAlbedoTex();//m_LBufferData.DirectLighting;
    m_planeModel.materials[0].maps[MATERIAL_MAP_OCCLUSION].texture = m_GBuffer_ptr->GetWorldPosTex();
    m_planeModel.materials[0].maps[MATERIAL_MAP_NORMAL].texture = m_GBuffer_ptr->GetNormalTex();
    m_planeModel.materials[0].maps[MATERIAL_MAP_METALNESS].texture = m_GBuffer_ptr->GetMetalnessTex();
    m_planeModel.materials[0].maps[MATERIAL_MAP_ROUGHNESS].texture = m_GBuffer_ptr->GetRoughnessTex();
    m_planeModel.materials[0].shader = m_DirectionalLightShader.shader;

    Vector3 pos = {0.0f, 1.0f, 0.0f};
    Vector3 dir = {0.0f, -1.0f, 0.0f};
    Vector3 up = {0.0f, 0.0f, 1.0f};
    m_dirLightCam.SetProjection(CAMERA_ORTHOGRAPHIC);
    m_dirLightCam.SetFOV(1.0f);
    m_dirLightCam.SetCameraPosition(pos);
    m_dirLightCam.LookAt(dir, up);
}


void LBuffer::UpdateLBuffer(Camera3D& cameraIn)
{
    BindForWriting();

    rlSetUniformSampler(SHADER_LOC_MAP_OCCLUSION, m_GBuffer_ptr->GetWorldPosTexID());
    rlSetUniformSampler(SHADER_LOC_MAP_NORMAL, m_GBuffer_ptr->GetNormalTexID());
    rlSetUniformSampler(SHADER_LOC_MAP_ALBEDO, m_GBuffer_ptr->GetAlbedoTexID());
    rlSetUniformSampler(SHADER_LOC_MAP_METALNESS, m_GBuffer_ptr->GetMetalnessTexID());
    rlSetUniformSampler(SHADER_LOC_MAP_ROUGHNESS, m_GBuffer_ptr->GetRoughnessTexID());
    rlSetUniformSampler(SHADER_LOC_MAP_EMISSION, m_GBuffer_ptr->GetEmissiveTexID());
    rlSetUniformSampler(SHADER_LOC_MAP_BRDF, m_LBufferData.DirectLighting.id);

    CalcDirectLighting(cameraIn);
    CalcIndirectLighting(cameraIn);

    BindForReading();
}

void LBuffer::CalcDirectLighting(Camera3D& cameraIn)
{
    float camPos[3] = { -cameraIn.position.x, cameraIn.position.y, cameraIn.position.z };
    SetShaderValue(m_DirectionalLightShader.shader, m_DirectionalLightShader.cameraPositionLoc, camPos, RL_SHADER_UNIFORM_VEC3);
    //SetShaderValueV(m_PointLightShader.shader, m_PointLightShader.cameraPositionLoc, &cameraIn.position, RL_SHADER_UNIFORM_VEC3, 3);
    //SetShaderValueV(m_SpotLightShader.shader, m_SpotLightShader.cameraPositionLoc, &cameraIn.position, RL_SHADER_UNIFORM_VEC3, 3);

    // Begin directional lights
    //BeginShaderMode(m_DirectionalLightShader.shader);
    BeginMode3D(*m_dirLightCam.GetCameraPTR());
    ClearBackground(LIGHTGRAY);
    Vector3 zeros = {0.0f, 0.0f, 0.0f};
    for (unsigned int i = 0; i < m_DirectionalLightContainer.size(); i++)
    {
        Vector2 screenSize = { m_LBufferData.screenWidth, m_LBufferData.screenHeight };

        float color[3] = { m_DirectionalLightContainer[i].color.x, m_DirectionalLightContainer[i].color.y, m_DirectionalLightContainer[i].color.z};
        m_DirectionalLightContainer[i].direction = Vector3Normalize(m_DirectionalLightContainer[i].direction);
        float direction[3] = { -m_DirectionalLightContainer[i].direction.x, m_DirectionalLightContainer[i].direction.y, m_DirectionalLightContainer[i].direction.z};
        float ambient[1] = { m_DirectionalLightContainer[i].ambientIntensity };
        float diffuse[1] = { m_DirectionalLightContainer[i].diffuseIntensity };

        SetShaderValue(m_DirectionalLightShader.shader, m_DirectionalLightShader.colorLoc, color, RL_SHADER_UNIFORM_VEC3);
        SetShaderValue(m_DirectionalLightShader.shader, m_DirectionalLightShader.ambientIntensityLoc, ambient, RL_SHADER_UNIFORM_FLOAT);
        SetShaderValue(m_DirectionalLightShader.shader, m_DirectionalLightShader.diffuseIntensityLoc, diffuse, RL_SHADER_UNIFORM_FLOAT);
        SetShaderValue(m_DirectionalLightShader.shader, m_DirectionalLightShader.directionLoc, direction, RL_SHADER_UNIFORM_VEC3);

        DrawModel(m_planeModel, zeros, 1.0f, WHITE);
        //DrawTextureRec(m_LBufferData.DirectLighting, Rectangle{ 0, 0, (float)m_LBufferData.DirectLighting.width, (float)-m_LBufferData.DirectLighting.height }, Vector2{ 0, 0 }, WHITE);
    }

    EndMode3D();
    //EndShaderMode();
    // End directional lights

    // Begin point and spot lights
    BeginMode3D(cameraIn);

    for (unsigned i = 0; i < m_PointLightContainer.size(); i++)
    {
        m_isosphereModel.materials[0] = m_spotLightShaderMat;
    }

    for (unsigned int i = 0; i < m_SpotLightContainer.size(); i++)
    {
        m_coneModel.materials[0] = m_spotLightShaderMat;
    }

    EndMode3D();
    // End point and spot lights
}

void LBuffer::CalcIndirectLighting(Camera3D& cameraIn)
{
}

void LBuffer::BindForWriting()
{
    rlDrawRenderBatchActive();      // Update and draw internal render batch

    rlEnableFramebuffer(m_LBufferData.bufferId); // Enable render target

    rlActiveDrawBuffers(3);

    // Set viewport and RLGL internal framebuffer size
    rlViewport(0, 0, m_LBufferData.screenWidth, m_LBufferData.screenHeight);

    rlMatrixMode(RL_PROJECTION);    // Switch to projection matrix
    rlLoadIdentity();               // Reset current matrix (projection)

    // Set orthographic projection to current framebuffer size
    // NOTE: Configured top-left corner as (0, 0)
    rlOrtho(0, m_LBufferData.screenWidth, m_LBufferData.screenHeight, 0, 0.0f, 1000.0f);

    rlMatrixMode(RL_MODELVIEW);     // Switch back to modelview matrix
    rlLoadIdentity();               // Reset current matrix (modelview)

    ClearBackground(BLACK);
}

void LBuffer::BindForReading()
{
    rlDrawRenderBatchActive();      // Update and draw internal render batch

    rlDisableFramebuffer();         // Disable render target (fbo)
}
