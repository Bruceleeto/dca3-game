GIT_VERSION := $(shell git describe --always --tags --long --dirty 2>/dev/null || echo "NO_GIT")
CI_JOB_ID ?= 00000000


git-version/git-version.tmp:
	@echo "Generating git-version.tmp with GIT_VERSION = \"$(GIT_VERSION)\""
	@mkdir -p git-version
	@echo "#pragma once" > git-version/git-version.tmp
	@echo "#ifndef VERSION_H" >> git-version/git-version.tmp
	@echo "#define VERSION_H" >> git-version/git-version.tmp
	@echo "#define GIT_VERSION \"$(GIT_VERSION)\"" >> git-version/git-version.tmp
	@echo "#define CI_JOB_ID \"$(CI_JOB_ID)\"" >> git-version/git-version.tmp
	@echo "#endif // VERSION_H" >> git-version/git-version.tmp

git-version/git-version.h: git-version/git-version.tmp
	@if [ ! -f git-version/git-version.h ] || ! cmp -s git-version/git-version.tmp git-version/git-version.h; then \
	  echo "Updating git-version.h"; \
	  cp git-version/git-version.tmp git-version/git-version.h; \
    else \
	  echo "git-version.h is up to date. No change."; \
	fi

.PHONY: git-version/git-version.tmp

../src/miami/skel/dc/dc.cpp: git-version/git-version.h



# List all of your C files here, but change the extension to ".o"
# Include "romdisk.o" if you want a rom disk.
RE3_OBJS = \
	../src/miami/animation/AnimBlendAssocGroup.o \
	../src/miami/animation/AnimBlendAssociation.o \
	../src/miami/animation/AnimBlendClumpData.o \
	../src/miami/animation/AnimBlendHierarchy.o \
	../src/miami/animation/AnimBlendNode.o \
	../src/miami/animation/AnimBlendSequence.o \
	../src/miami/animation/AnimManager.o \
	../src/miami/animation/Bones.o \
	../src/miami/animation/CutsceneMgr.o \
	../src/miami/animation/FrameUpdate.o \
	../src/miami/animation/RpAnimBlend.o \
	\
	../src/miami/buildings/Building.o \
	../src/miami/buildings/Treadable.o \
	\
	../src/miami/collision/ColBox.o \
	../src/miami/collision/ColLine.o \
	../src/miami/collision/Collision.o \
	../src/miami/collision/ColModel.o \
	../src/miami/collision/ColPoint.o \
	../src/miami/collision/ColSphere.o \
	../src/miami/collision/ColStore.o \
	../src/miami/collision/ColTriangle.o \
	../src/miami/collision/TempColModels.o \
	../src/miami/collision/VuCollision.o \
	\
	../src/miami/control/AutoPilot.o \
	../src/miami/control/Bridge.o \
	../src/miami/control/CarAI.o \
	../src/miami/control/CarCtrl.o \
	../src/miami/control/Curves.o \
	../src/miami/control/Darkel.o \
	../src/miami/control/GameLogic.o \
	../src/miami/control/Garages.o \
	../src/miami/control/NameGrid.o \
	../src/miami/control/OnscreenTimer.o \
	../src/miami/control/PathFind.o \
	../src/miami/control/Phones.o \
	../src/miami/control/Pickups.o \
	../src/miami/control/PowerPoints.o \
	../src/miami/control/Record.o \
	../src/miami/control/Remote.o \
	../src/miami/control/Replay.o \
	../src/miami/control/Restart.o \
	../src/miami/control/RoadBlocks.o \
	../src/miami/control/SceneEdit.o \
	../src/miami/control/Script.o \
	../src/miami/control/Script2.o \
	../src/miami/control/Script3.o \
	../src/miami/control/Script4.o \
	../src/miami/control/Script5.o \
	../src/miami/control/Script6.o \
	../src/miami/control/Script7.o \
	../src/miami/control/Script8.o \
	../src/miami/control/SetPieces.o \
	../src/miami/control/ScriptDebug.o \
	../src/miami/control/TrafficLights.o \
	\
	../src/miami/core/Accident.o \
	../src/miami/core/Cam.o \
	../src/miami/core/Camera.o \
	../src/miami/core/CdStreamDC.o \
	../src/miami/core/Clock.o \
	../src/miami/core/ControllerConfig.o \
	../src/miami/core/Debug.o \
	../src/miami/core/Directory.o \
	../src/miami/core/EventList.o \
	../src/miami/core/FileLoader.o \
	../src/miami/core/FileMgr.o \
	../src/miami/core/Fire.o \
	../src/miami/core/Frontend.o \
	../src/miami/core/FrontEndControls.o \
	../src/miami/core/Frontend_PS2.o \
	../src/miami/core/Game.o \
	../src/miami/core/IniFile.o \
	../src/miami/core/Lists.o \
	../src/miami/core/main.o \
	../src/miami/core/MenuScreens.o \
	../src/miami/core/MenuScreensCustom.o \
	../src/miami/core/obrstr.o \
	../src/miami/core/Pad.o \
	../src/miami/core/Placeable.o \
	../src/miami/core/PlayerInfo.o \
	../src/miami/core/Pools.o \
	../src/miami/core/Profile.o \
	../src/miami/core/Radar.o \
	../src/miami/core/Range2D.o \
	../src/miami/core/Range3D.o \
	../src/miami/core/re3.o \
	../src/miami/core/References.o \
	../src/miami/core/Ropes.o \
	../src/miami/core/Stats.o \
	../src/miami/core/Streaming.o \
	../src/miami/core/SurfaceTable.o \
	../src/miami/core/timebars.o \
	../src/miami/core/Timer.o \
	../src/miami/core/TimeStep.o \
	../src/miami/core/User.o \
	../src/miami/core/Wanted.o \
	../src/miami/core/World.o \
	../src/miami/core/ZoneCull.o \
	../src/miami/core/Zones.o \
	\
	../src/miami/entities/Dummy.o \
	../src/miami/entities/Entity.o \
	../src/miami/entities/Physical.o \
	\
	../src/miami/fakerw/fake.o \
	\
	../src/miami/math/math.o \
	../src/miami/math/Matrix.o \
	../src/miami/math/Quaternion.o \
	../src/miami/math/Rect.o \
	../src/miami/math/Vector.o \
	\
	../src/miami/modelinfo/BaseModelInfo.o \
	../src/miami/modelinfo/ClumpModelInfo.o \
	../src/miami/modelinfo/MloModelInfo.o \
	../src/miami/modelinfo/ModelIndices.o \
	../src/miami/modelinfo/ModelInfo.o \
	../src/miami/modelinfo/PedModelInfo.o \
	../src/miami/modelinfo/SimpleModelInfo.o \
	../src/miami/modelinfo/TimeModelInfo.o \
	../src/miami/modelinfo/VehicleModelInfo.o \
	../src/miami/modelinfo/WeaponModelInfo.o \
	\
	../src/miami/objects/CutsceneObject.o \
	../src/miami/objects/DummyObject.o \
	../src/miami/objects/Object.o \
	../src/miami/objects/ObjectData.o \
	../src/miami/objects/ParticleObject.o \
	../src/miami/objects/Projectile.o \
	../src/miami/objects/Stinger.o \
	\
	../src/miami/peds/CivilianPed.o \
	../src/miami/peds/CopPed.o \
	../src/miami/peds/EmergencyPed.o \
	../src/miami/peds/Gangs.o \
	../src/miami/peds/Ped.o \
	../src/miami/peds/PedAI.o \
	../src/miami/peds/PedChat.o \
	../src/miami/peds/PedDebug.o \
	../src/miami/peds/PedFight.o \
	../src/miami/peds/PedIK.o \
	../src/miami/peds/PedAttractor.o \
	../src/miami/peds/PedPlacement.o \
	../src/miami/peds/PedRoutes.o \
	../src/miami/peds/PedType.o \
	../src/miami/peds/PlayerPed.o \
	../src/miami/peds/Population.o \
	\
	../src/miami/renderer/Antennas.o \
	../src/miami/renderer/Clouds.o \
	../src/miami/renderer/Console.o \
	../src/miami/renderer/Coronas.o \
	../src/miami/renderer/Credits.o \
	../src/miami/renderer/CutsceneShadow.o \
	../src/miami/renderer/Draw.o \
	../src/miami/renderer/Fluff.o \
	../src/miami/renderer/Font.o \
	../src/miami/renderer/Glass.o \
	../src/miami/renderer/Hud.o \
	../src/miami/renderer/Instance.o \
	../src/miami/renderer/Lines.o \
	../src/miami/renderer/MBlur.o \
	../src/miami/renderer/Occlusion.o \
	../src/miami/renderer/Particle.o \
	../src/miami/renderer/ParticleMgr.o \
	../src/miami/renderer/PlayerSkin.o \
	../src/miami/renderer/PointLights.o \
	../src/miami/renderer/RenderBuffer.o \
	../src/miami/renderer/Renderer.o \
	../src/miami/renderer/Rubbish.o \
	../src/miami/renderer/Shadows.o \
	../src/miami/renderer/ShadowCamera.o \
	../src/miami/renderer/Skidmarks.o \
	../src/miami/renderer/SpecialFX.o \
	../src/miami/renderer/Sprite.o \
	../src/miami/renderer/Sprite2d.o \
	../src/miami/renderer/TexList.o \
	../src/miami/renderer/Timecycle.o \
	../src/miami/renderer/VarConsole.o \
	../src/miami/renderer/WaterCannon.o \
	../src/miami/renderer/WaterCreatures.o \
	../src/miami/renderer/WaterLevel.o \
	../src/miami/renderer/Weather.o \
	../src/miami/renderer/WindModifiers.o \
	\
	../src/miami/rw/ClumpRead.o \
	../src/miami/rw/Lights.o \
	../src/miami/rw/MemoryHeap.o \
	../src/miami/rw/MemoryMgr.o \
	../src/miami/rw/NodeName.o \
	../src/miami/rw/RwHelper.o \
	../src/miami/rw/RwMatFX.o \
	../src/miami/rw/RwPS2AlphaTest.o \
	../src/miami/rw/TexRead.o \
	../src/miami/rw/TexturePools.o \
	../src/miami/rw/TxdStore.o \
	../src/miami/rw/VisibilityPlugins.o \
	\
	../src/miami/skel/crossplatform.o \
	../src/miami/skel/events.o \
	../src/miami/skel/skeleton.o \
	../src/miami/skel/dc/dc.o \
	\
	../src/miami/text/Messages.o \
	../src/miami/text/Pager.o \
	../src/miami/text/Text.o \
	\
	../src/miami/vehicles/Automobile.o \
	../src/miami/vehicles/Boat.o \
	../src/miami/vehicles/Bike.o \
	../src/miami/vehicles/CarGen.o \
	../src/miami/vehicles/Cranes.o \
	../src/miami/vehicles/DamageManager.o \
	../src/miami/vehicles/Door.o \
	../src/miami/vehicles/Floater.o \
	../src/miami/vehicles/HandlingMgr.o \
	../src/miami/vehicles/Heli.o \
	../src/miami/vehicles/Plane.o \
	../src/miami/vehicles/Train.o \
	../src/miami/vehicles/Transmission.o \
	../src/miami/vehicles/Vehicle.o \
	\
	../src/miami/weapons/BulletInfo.o \
	../src/miami/weapons/Explosion.o \
	../src/miami/weapons/ProjectileInfo.o \
	../src/miami/weapons/ShotInfo.o \
	../src/miami/weapons/Weapon.o \
	../src/miami/weapons/WeaponEffects.o \
	../src/miami/weapons/WeaponInfo.o \
	\
	../src/miami/audio/AudioCollision.o \
	../src/miami/audio/AudioLogic.o \
	../src/miami/audio/AudioManager.o \
	../src/miami/audio/AudioScriptObject.o \
	../src/miami/audio/DMAudio.o \
	../src/miami/audio/MusicManager.o \
	../src/miami/audio/PolRadio.o \
	../src/miami/audio/sampman_miles.o \
	../src/miami/audio/sampman_oal.o \
	\
	../src/miami/save/Date.o \
	../src/miami/save/GenericGameStorage.o \
	../src/miami/save/MemoryCard.o \
	../src/miami/save/PCSave.o \
	\
	../src/miami/extras/debugmenu.o \
	../src/miami/extras/frontendoption.o \
	../src/miami/extras/postfx.o \
	../src/miami/extras/screendroplets.o \
	\
	../vendor/miniLZO/minilzo.o \
	\
	../src/common/vmu/vmu.o \
	\
	../src/common/thread/thread.o \
	\
	../vendor/tlsf/tlsf.o

# Excluded \
	../src/miami/extras/custompipes.o \
	../src/miami/extras/custompipes_d3d9.o \
	../src/miami/extras/custompipes_gl.o \
	../src/miami/core/CdStream.o \
	../src/miami/core/CdStreamPosix.o \
	../src/miami/extras \
	../src/miami/extras/GitSHA1.cpp.in \
	../src/miami/core/AnimViewer.o \

RW_OBJS = \
    ../vendor/librw/src/anim.o \
    ../vendor/librw/src/base.o \
    ../vendor/librw/src/camera.o \
    ../vendor/librw/src/charset.o \
    ../vendor/librw/src/clump.o \
    ../vendor/librw/src/engine.o \
    ../vendor/librw/src/error.o \
    ../vendor/librw/src/frame.o \
    ../vendor/librw/src/geometry.o \
    ../vendor/librw/src/geoplg.o \
    ../vendor/librw/src/hanim.o \
    ../vendor/librw/src/image.o \
    ../vendor/librw/src/light.o \
    ../vendor/librw/src/matfx.o \
    ../vendor/librw/src/pipeline.o \
    ../vendor/librw/src/plg.o \
    ../vendor/librw/src/prim.o \
    ../vendor/librw/src/raster.o \
    ../vendor/librw/src/render.o \
    ../vendor/librw/src/skin.o \
    ../vendor/librw/src/texture.o \
    ../vendor/librw/src/tristrip.o \
    ../vendor/librw/src/userdata.o \
    ../vendor/librw/src/uvanim.o \
    ../vendor/librw/src/world.o \
	\
	../vendor/librw/src/dc/rwdc.o \
	../vendor/librw/src/dc/alloc.o

# Excluded \
	../vendor/librw/src/d3d-x/d3d.o \
	../vendor/librw/src/d3d-x/d3d8.o \
	../vendor/librw/src/d3d-x/d3d8render.o \
	../vendor/librw/src/d3d/d3d8.o \
    ../vendor/librw/src/d3d/d3d8matfx.o \
    ../vendor/librw/src/d3d/d3d8render.o \
    ../vendor/librw/src/d3d/d3d8skin.o \
    ../vendor/librw/src/d3d/d3d9.o \
    ../vendor/librw/src/d3d/d3d9matfx.o \
    ../vendor/librw/src/d3d/d3d9render.o \
    ../vendor/librw/src/d3d/d3d9skin.o \
    ../vendor/librw/src/d3d/d3d.o \
    ../vendor/librw/src/d3d/d3ddevice.o \
    ../vendor/librw/src/d3d/d3dimmed.o \
    ../vendor/librw/src/d3d/d3drender.o \
    ../vendor/librw/src/d3d/xbox.o \
    ../vendor/librw/src/d3d/xboxmatfx.o \
    ../vendor/librw/src/d3d/xboxskin.o \
    ../vendor/librw/src/d3d/xboxvfmt.o \
	\
    ../vendor/librw/src/gl/gl3.o \
    ../vendor/librw/src/gl/gl3device.o \
    ../vendor/librw/src/gl/gl3immed.o \
    ../vendor/librw/src/gl/gl3matfx.o \
    ../vendor/librw/src/gl/gl3pipe.o \
    ../vendor/librw/src/gl/gl3raster.o \
    ../vendor/librw/src/gl/gl3render.o \
    ../vendor/librw/src/gl/gl3shader.o \
    ../vendor/librw/src/gl/gl3skin.o \
    ../vendor/librw/src/gl/wdgl.o \
    ../vendor/librw/src/gl/glad/glad.cXXX \
	\
    ../vendor/librw/src/ps2/pds.o \
    ../vendor/librw/src/ps2/ps2.o \
    ../vendor/librw/src/ps2/ps2device.o \
    ../vendor/librw/src/ps2/ps2matfx.o \
    ../vendor/librw/src/ps2/ps2raster.o \
    ../vendor/librw/src/ps2/ps2skin.o \

INCLUDE = \
-I../src/miami/animation \
-I../src/miami/audio \
-I../src/miami/buildings \
-I../src/miami/collision \
-I../src/miami/control \
-I../src/miami/core \
-I../src/miami/entities \
-I../src/miami/extras \
-I../src/miami/fakerw \
-I../src/miami/math \
-I../src/miami/modelinfo \
-I../src/miami/objects \
-I../src/miami/peds \
-I../src/miami/renderer \
-I../src/miami/rw \
-I../src/miami/save \
-I../src/miami/skel \
-I../src/miami/text \
-I../src/miami/vehicles \
-I../src/miami/weapons \
-I../src/miami/audio/eax \
-I../src/miami/audio/oal \
-I../src/miami/extras/shaders \
-I../src/miami/extras/shaders/obj \
-I../src/miami/skel/glfw \
-I../src/miami/skel/win \
\
-I../vendor/librw \
\
-I../vendor/miniLZO \
\
-I../src/common \
\
-Igit-version \
\
-I../vendor/tlsf

DEFINES = -DRW_DC -DLIBRW $(if $(WITH_LOGGING),-DWITH_LOGGING) $(if $(WITH_DCLOAD),-DDC_CHDIR=/pc) \
	$(if $(WITH_BEEPS),-DWITH_BEEPS)
FLAGS = -fpermissive -Wno-sign-compare -Wno-parentheses -Wno-maybe-uninitialized \
	-Wno-format -Wno-strict-aliasing -Wno-unused-variable \
	-Wno-unused-but-set-variable -Wno-write-strings \
	-Wno-deprecated-enum-enum-conversion -Wno-deprecated-enum-float-conversion \
	-Wno-multichar -Wno-unused-value -Wno-char-subscripts -Wno-reorder \
	-Wno-unused-function -Wno-class-memaccess -fno-permissive

CPPFLAGS += $(INCLUDE) $(DEFINES) $(FLAGS)
CFLAGS += -std=gnu17 $(CPPFLAGS)
CXXFLAGS += -std=gnu++20 $(CPPFLAGS)