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

../src/liberty/skel/dc/dc.cpp: git-version/git-version.h


# List all of your C files here, but change the extension to ".o"
# Include "romdisk.o" if you want a rom disk.
RE3_OBJS = \
	../src/liberty/animation/AnimBlendAssocGroup.o \
	../src/liberty/animation/AnimBlendAssociation.o \
	../src/liberty/animation/AnimBlendClumpData.o \
	../src/liberty/animation/AnimBlendHierarchy.o \
	../src/liberty/animation/AnimBlendNode.o \
	../src/liberty/animation/AnimBlendSequence.o \
	../src/liberty/animation/AnimManager.o \
	../src/liberty/animation/Bones.o \
	../src/liberty/animation/CutsceneMgr.o \
	../src/liberty/animation/FrameUpdate.o \
	../src/liberty/animation/RpAnimBlend.o \
	\
	../src/liberty/buildings/Building.o \
	../src/liberty/buildings/Treadable.o \
	\
	../src/liberty/collision/ColBox.o \
	../src/liberty/collision/ColLine.o \
	../src/liberty/collision/Collision.o \
	../src/liberty/collision/ColModel.o \
	../src/liberty/collision/ColPoint.o \
	../src/liberty/collision/ColSphere.o \
	../src/liberty/collision/ColTriangle.o \
	../src/liberty/collision/TempColModels.o \
	../src/liberty/collision/VuCollision.o \
	\
	../src/liberty/control/AutoPilot.o \
	../src/liberty/control/Bridge.o \
	../src/liberty/control/CarAI.o \
	../src/liberty/control/CarCtrl.o \
	../src/liberty/control/Curves.o \
	../src/liberty/control/Darkel.o \
	../src/liberty/control/GameLogic.o \
	../src/liberty/control/Garages.o \
	../src/liberty/control/NameGrid.o \
	../src/liberty/control/OnscreenTimer.o \
	../src/liberty/control/PathFind.o \
	../src/liberty/control/Phones.o \
	../src/liberty/control/Pickups.o \
	../src/liberty/control/PowerPoints.o \
	../src/liberty/control/Record.o \
	../src/liberty/control/Remote.o \
	../src/liberty/control/Replay.o \
	../src/liberty/control/Restart.o \
	../src/liberty/control/RoadBlocks.o \
	../src/liberty/control/SceneEdit.o \
	../src/liberty/control/Script.o \
	../src/liberty/control/Script2.o \
	../src/liberty/control/Script3.o \
	../src/liberty/control/Script4.o \
	../src/liberty/control/Script5.o \
	../src/liberty/control/Script6.o \
	../src/liberty/control/ScriptDebug.o \
	../src/liberty/control/TrafficLights.o \
	\
	../src/liberty/core/Accident.o \
	../src/liberty/core/Cam.o \
	../src/liberty/core/Camera.o \
	../src/liberty/core/CdStreamDC.o \
	../src/liberty/core/Clock.o \
	../src/liberty/core/ControllerConfig.o \
	../src/liberty/core/Debug.o \
	../src/liberty/core/Directory.o \
	../src/liberty/core/EventList.o \
	../src/liberty/core/FileLoader.o \
	../src/liberty/core/FileMgr.o \
	../src/liberty/core/Fire.o \
	../src/liberty/core/Frontend.o \
	../src/liberty/core/FrontEndControls.o \
	../src/liberty/core/Frontend_PS2.o \
	../src/liberty/core/Game.o \
	../src/liberty/core/IniFile.o \
	../src/liberty/core/Lists.o \
	../src/liberty/core/main.o \
	../src/liberty/core/MenuScreens.o \
	../src/liberty/core/MenuScreensCustom.o \
	../src/liberty/core/obrstr.o \
	../src/liberty/core/Pad.o \
	../src/liberty/core/Placeable.o \
	../src/liberty/core/PlayerInfo.o \
	../src/liberty/core/Pools.o \
	../src/liberty/core/Profile.o \
	../src/liberty/core/Radar.o \
	../src/liberty/core/Range2D.o \
	../src/liberty/core/Range3D.o \
	../src/liberty/core/re3.o \
	../src/liberty/core/References.o \
	../src/liberty/core/Stats.o \
	../src/liberty/core/Streaming.o \
	../src/liberty/core/SurfaceTable.o \
	../src/liberty/core/timebars.o \
	../src/liberty/core/Timer.o \
	../src/liberty/core/TimeStep.o \
	../src/liberty/core/User.o \
	../src/liberty/core/Wanted.o \
	../src/liberty/core/World.o \
	../src/liberty/core/ZoneCull.o \
	../src/liberty/core/Zones.o \
	\
	../src/liberty/entities/Dummy.o \
	../src/liberty/entities/Entity.o \
	../src/liberty/entities/Physical.o \
	\
	../src/liberty/fakerw/fake.o \
	\
	../src/liberty/math/math.o \
	../src/liberty/math/Matrix.o \
	../src/liberty/math/Quaternion.o \
	../src/liberty/math/Rect.o \
	../src/liberty/math/Vector.o \
	\
	../src/liberty/modelinfo/BaseModelInfo.o \
	../src/liberty/modelinfo/ClumpModelInfo.o \
	../src/liberty/modelinfo/MloModelInfo.o \
	../src/liberty/modelinfo/ModelIndices.o \
	../src/liberty/modelinfo/ModelInfo.o \
	../src/liberty/modelinfo/PedModelInfo.o \
	../src/liberty/modelinfo/SimpleModelInfo.o \
	../src/liberty/modelinfo/TimeModelInfo.o \
	../src/liberty/modelinfo/VehicleModelInfo.o \
	\
	../src/liberty/objects/CutsceneHead.o \
	../src/liberty/objects/CutsceneObject.o \
	../src/liberty/objects/DummyObject.o \
	../src/liberty/objects/Object.o \
	../src/liberty/objects/ObjectData.o \
	../src/liberty/objects/ParticleObject.o \
	../src/liberty/objects/Projectile.o \
	\
	../src/liberty/peds/CivilianPed.o \
	../src/liberty/peds/CopPed.o \
	../src/liberty/peds/EmergencyPed.o \
	../src/liberty/peds/Gangs.o \
	../src/liberty/peds/Ped.o \
	../src/liberty/peds/PedAI.o \
	../src/liberty/peds/PedChat.o \
	../src/liberty/peds/PedDebug.o \
	../src/liberty/peds/PedFight.o \
	../src/liberty/peds/PedIK.o \
	../src/liberty/peds/PedPlacement.o \
	../src/liberty/peds/PedRoutes.o \
	../src/liberty/peds/PedType.o \
	../src/liberty/peds/PlayerPed.o \
	../src/liberty/peds/Population.o \
	\
	../src/liberty/renderer/Antennas.o \
	../src/liberty/renderer/Clouds.o \
	../src/liberty/renderer/Console.o \
	../src/liberty/renderer/Coronas.o \
	../src/liberty/renderer/Credits.o \
	../src/liberty/renderer/Draw.o \
	../src/liberty/renderer/Fluff.o \
	../src/liberty/renderer/Font.o \
	../src/liberty/renderer/Glass.o \
	../src/liberty/renderer/Hud.o \
	../src/liberty/renderer/Instance.o \
	../src/liberty/renderer/Lines.o \
	../src/liberty/renderer/MBlur.o \
	../src/liberty/renderer/Particle.o \
	../src/liberty/renderer/ParticleMgr.o \
	../src/liberty/renderer/PlayerSkin.o \
	../src/liberty/renderer/PointLights.o \
	../src/liberty/renderer/RenderBuffer.o \
	../src/liberty/renderer/Renderer.o \
	../src/liberty/renderer/Rubbish.o \
	../src/liberty/renderer/Shadows.o \
	../src/liberty/renderer/Skidmarks.o \
	../src/liberty/renderer/SpecialFX.o \
	../src/liberty/renderer/Sprite.o \
	../src/liberty/renderer/Sprite2d.o \
	../src/liberty/renderer/TexList.o \
	../src/liberty/renderer/Timecycle.o \
	../src/liberty/renderer/WaterCannon.o \
	../src/liberty/renderer/WaterLevel.o \
	../src/liberty/renderer/Weather.o \
	\
	../src/liberty/rw/ClumpRead.o \
	../src/liberty/rw/Lights.o \
	../src/liberty/rw/MemoryHeap.o \
	../src/liberty/rw/MemoryMgr.o \
	../src/liberty/rw/NodeName.o \
	../src/liberty/rw/RwHelper.o \
	../src/liberty/rw/RwMatFX.o \
	../src/liberty/rw/RwPS2AlphaTest.o \
	../src/liberty/rw/TexRead.o \
	../src/liberty/rw/TexturePools.o \
	../src/liberty/rw/TxdStore.o \
	../src/liberty/rw/VisibilityPlugins.o \
	\
	../src/liberty/skel/crossplatform.o \
	../src/liberty/skel/events.o \
	../src/liberty/skel/skeleton.o \
	../src/liberty/skel/dc/dc.o \
	\
	../src/liberty/text/Messages.o \
	../src/liberty/text/Pager.o \
	../src/liberty/text/Text.o \
	\
	../src/liberty/vehicles/Automobile.o \
	../src/liberty/vehicles/Boat.o \
	../src/liberty/vehicles/CarGen.o \
	../src/liberty/vehicles/Cranes.o \
	../src/liberty/vehicles/DamageManager.o \
	../src/liberty/vehicles/Door.o \
	../src/liberty/vehicles/Floater.o \
	../src/liberty/vehicles/HandlingMgr.o \
	../src/liberty/vehicles/Heli.o \
	../src/liberty/vehicles/Plane.o \
	../src/liberty/vehicles/Train.o \
	../src/liberty/vehicles/Transmission.o \
	../src/liberty/vehicles/Vehicle.o \
	\
	../src/liberty/weapons/BulletInfo.o \
	../src/liberty/weapons/Explosion.o \
	../src/liberty/weapons/ProjectileInfo.o \
	../src/liberty/weapons/ShotInfo.o \
	../src/liberty/weapons/Weapon.o \
	../src/liberty/weapons/WeaponEffects.o \
	../src/liberty/weapons/WeaponInfo.o \
	\
	../src/liberty/audio/AudioCollision.o \
	../src/liberty/audio/AudioLogic.o \
	../src/liberty/audio/AudioManager.o \
	../src/liberty/audio/AudioScriptObject.o \
	../src/liberty/audio/DMAudio.o \
	../src/liberty/audio/MusicManager.o \
	../src/liberty/audio/PolRadio.o \
	../src/liberty/audio/sampman_miles.o \
	../src/liberty/audio/sampman_oal.o \
	\
	../src/liberty/save/Date.o \
	../src/liberty/save/GenericGameStorage.o \
	../src/liberty/save/MemoryCard.o \
	../src/liberty/save/PCSave.o \
	\
	../src/liberty/extras/debugmenu.o \
	../src/liberty/extras/frontendoption.o \
	../src/liberty/extras/postfx.o \
	../src/liberty/extras/screendroplets.o \
	\
	../src/common/vmu/vmu.o \
	../vendor/miniLZO/minilzo.o \
	\

# Excluded \
	../src/liberty/extras/custompipes.o \
	../src/liberty/extras/custompipes_d3d9.o \
	../src/liberty/extras/custompipes_gl.o \
	../src/liberty/core/CdStream.o \
	../src/liberty/core/CdStreamPosix.o \
	../src/liberty/extras \
	../src/liberty/extras/GitSHA1.cpp.in \
	../src/liberty/core/AnimViewer.o \

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
-I../src/liberty/animation \
-I../src/liberty/audio \
-I../src/liberty/buildings \
-I../src/liberty/collision \
-I../src/liberty/control \
-I../src/liberty/core \
-I../src/liberty/entities \
-I../src/liberty/extras \
-I../src/liberty/fakerw \
-I../src/liberty/math \
-I../src/liberty/modelinfo \
-I../src/liberty/objects \
-I../src/liberty/peds \
-I../src/liberty/renderer \
-I../src/liberty/rw \
-I../src/liberty/save \
-I../src/liberty/skel \
-I../src/liberty/text \
-I../src/liberty/vehicles \
-I../src/liberty/weapons \
-I../src/liberty/audio/eax \
-I../src/liberty/audio/oal \
-I../src/liberty/extras/shaders \
-I../src/liberty/extras/shaders/obj \
-I../src/liberty/skel/glfw \
-I../src/liberty/skel/win \
\
-I../vendor/librw \
\
-I../vendor/miniLZO \
\
-I../src/common \
\
-Igit-version

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