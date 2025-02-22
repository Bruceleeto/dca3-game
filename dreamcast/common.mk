GIT_VERSION := $(shell git describe --always --tags --long --dirty 2>/dev/null || echo "NO_GIT")
CI_JOB_ID ?= 00000000


git-version.tmp:
	@echo "Generating git-version.tmp with GIT_VERSION = \"$(GIT_VERSION)\""
	@echo "#pragma once" > git-version.tmp
	@echo "#ifndef VERSION_H" >> git-version.tmp
	@echo "#define VERSION_H" >> git-version.tmp
	@echo "#define GIT_VERSION \"$(GIT_VERSION)\"" >> git-version.tmp
	@echo "#define CI_JOB_ID \"$(CI_JOB_ID)\"" >> git-version.tmp
	@echo "#endif // VERSION_H" >> git-version.tmp

git-version.h: git-version.tmp
	@if [ ! -f git-version.h ] || ! cmp -s git-version.tmp git-version.h; then \
	  echo "Updating git-version.h"; \
	  cp git-version.tmp git-version.h; \
    else \
	  echo "git-version.h is up to date. No change."; \
	fi

.PHONY: git-version.tmp

../miami/skel/dc/dc.cpp: git-version.h


# ../miami/core/CdStreamDC.o \
# ../miami/skel/dc/dc.o \
../miami/vmu/vmu.o \

# List all of your C files here, but change the extension to ".o"
# Include "romdisk.o" if you want a rom disk.
RE3_OBJS = \
	../miami/animation/AnimBlendAssocGroup.o \
	../miami/animation/AnimBlendAssociation.o \
	../miami/animation/AnimBlendClumpData.o \
	../miami/animation/AnimBlendHierarchy.o \
	../miami/animation/AnimBlendNode.o \
	../miami/animation/AnimBlendSequence.o \
	../miami/animation/AnimManager.o \
	../miami/animation/Bones.o \
	../miami/animation/CutsceneMgr.o \
	../miami/animation/FrameUpdate.o \
	../miami/animation/RpAnimBlend.o \
	\
	../miami/buildings/Building.o \
	../miami/buildings/Treadable.o \
	\
	../miami/collision/ColBox.o \
	../miami/collision/ColLine.o \
	../miami/collision/Collision.o \
	../miami/collision/ColModel.o \
	../miami/collision/ColPoint.o \
	../miami/collision/ColSphere.o \
	../miami/collision/ColTriangle.o \
	../miami/collision/TempColModels.o \
	../miami/collision/VuCollision.o \
	\
	../miami/control/AutoPilot.o \
	../miami/control/Bridge.o \
	../miami/control/CarAI.o \
	../miami/control/CarCtrl.o \
	../miami/control/Curves.o \
	../miami/control/Darkel.o \
	../miami/control/GameLogic.o \
	../miami/control/Garages.o \
	../miami/control/NameGrid.o \
	../miami/control/OnscreenTimer.o \
	../miami/control/PathFind.o \
	../miami/control/Phones.o \
	../miami/control/Pickups.o \
	../miami/control/PowerPoints.o \
	../miami/control/Record.o \
	../miami/control/Remote.o \
	../miami/control/Replay.o \
	../miami/control/Restart.o \
	../miami/control/RoadBlocks.o \
	../miami/control/SceneEdit.o \
	../miami/control/Script.o \
	../miami/control/Script2.o \
	../miami/control/Script3.o \
	../miami/control/Script4.o \
	../miami/control/Script5.o \
	../miami/control/Script6.o \
	../miami/control/ScriptDebug.o \
	../miami/control/TrafficLights.o \
	\
	../miami/core/Accident.o \
	../miami/core/Cam.o \
	../miami/core/Camera.o \
	../miami/core/Clock.o \
	../miami/core/ControllerConfig.o \
	../miami/core/Debug.o \
	../miami/core/Directory.o \
	../miami/core/EventList.o \
	../miami/core/FileLoader.o \
	../miami/core/FileMgr.o \
	../miami/core/Fire.o \
	../miami/core/Frontend.o \
	../miami/core/FrontEndControls.o \
	../miami/core/Frontend_PS2.o \
	../miami/core/Game.o \
	../miami/core/IniFile.o \
	../miami/core/Lists.o \
	../miami/core/main.o \
	../miami/core/MenuScreens.o \
	../miami/core/MenuScreensCustom.o \
	../miami/core/obrstr.o \
	../miami/core/Pad.o \
	../miami/core/Placeable.o \
	../miami/core/PlayerInfo.o \
	../miami/core/Pools.o \
	../miami/core/Profile.o \
	../miami/core/Radar.o \
	../miami/core/Range2D.o \
	../miami/core/Range3D.o \
	../miami/core/re3.o \
	../miami/core/References.o \
	../miami/core/Stats.o \
	../miami/core/Streaming.o \
	../miami/core/SurfaceTable.o \
	../miami/core/timebars.o \
	../miami/core/Timer.o \
	../miami/core/TimeStep.o \
	../miami/core/User.o \
	../miami/core/Wanted.o \
	../miami/core/World.o \
	../miami/core/ZoneCull.o \
	../miami/core/Zones.o \
	\
	../miami/entities/Dummy.o \
	../miami/entities/Entity.o \
	../miami/entities/Physical.o \
	\
	../miami/fakerw/fake.o \
	\
	../miami/math/math.o \
	../miami/math/Matrix.o \
	../miami/math/Quaternion.o \
	../miami/math/Rect.o \
	../miami/math/Vector.o \
	\
	../miami/modelinfo/BaseModelInfo.o \
	../miami/modelinfo/ClumpModelInfo.o \
	../miami/modelinfo/MloModelInfo.o \
	../miami/modelinfo/ModelIndices.o \
	../miami/modelinfo/ModelInfo.o \
	../miami/modelinfo/PedModelInfo.o \
	../miami/modelinfo/SimpleModelInfo.o \
	../miami/modelinfo/TimeModelInfo.o \
	../miami/modelinfo/VehicleModelInfo.o \
	\
	../miami/objects/CutsceneObject.o \
	../miami/objects/DummyObject.o \
	../miami/objects/Object.o \
	../miami/objects/ObjectData.o \
	../miami/objects/ParticleObject.o \
	../miami/objects/Projectile.o \
	\
	../miami/peds/CivilianPed.o \
	../miami/peds/CopPed.o \
	../miami/peds/EmergencyPed.o \
	../miami/peds/Gangs.o \
	../miami/peds/Ped.o \
	../miami/peds/PedAI.o \
	../miami/peds/PedChat.o \
	../miami/peds/PedDebug.o \
	../miami/peds/PedFight.o \
	../miami/peds/PedIK.o \
	../miami/peds/PedPlacement.o \
	../miami/peds/PedRoutes.o \
	../miami/peds/PedType.o \
	../miami/peds/PlayerPed.o \
	../miami/peds/Population.o \
	\
	../miami/renderer/Antennas.o \
	../miami/renderer/Clouds.o \
	../miami/renderer/Console.o \
	../miami/renderer/Coronas.o \
	../miami/renderer/Credits.o \
	../miami/renderer/Draw.o \
	../miami/renderer/Fluff.o \
	../miami/renderer/Font.o \
	../miami/renderer/Glass.o \
	../miami/renderer/Hud.o \
	../miami/renderer/Instance.o \
	../miami/renderer/Lines.o \
	../miami/renderer/MBlur.o \
	../miami/renderer/Particle.o \
	../miami/renderer/ParticleMgr.o \
	../miami/renderer/PlayerSkin.o \
	../miami/renderer/PointLights.o \
	../miami/renderer/RenderBuffer.o \
	../miami/renderer/Renderer.o \
	../miami/renderer/Rubbish.o \
	../miami/renderer/Shadows.o \
	../miami/renderer/Skidmarks.o \
	../miami/renderer/SpecialFX.o \
	../miami/renderer/Sprite.o \
	../miami/renderer/Sprite2d.o \
	../miami/renderer/TexList.o \
	../miami/renderer/Timecycle.o \
	../miami/renderer/WaterCannon.o \
	../miami/renderer/WaterLevel.o \
	../miami/renderer/Weather.o \
	\
	../miami/rw/ClumpRead.o \
	../miami/rw/Lights.o \
	../miami/rw/MemoryHeap.o \
	../miami/rw/MemoryMgr.o \
	../miami/rw/NodeName.o \
	../miami/rw/RwHelper.o \
	../miami/rw/RwMatFX.o \
	../miami/rw/RwPS2AlphaTest.o \
	../miami/rw/TexRead.o \
	../miami/rw/TexturePools.o \
	../miami/rw/TxdStore.o \
	../miami/rw/VisibilityPlugins.o \
	\
	../miami/skel/crossplatform.o \
	../miami/skel/events.o \
	../miami/skel/skeleton.o \
	\
	../miami/text/Messages.o \
	../miami/text/Pager.o \
	../miami/text/Text.o \
	\
	../miami/vehicles/Automobile.o \
	../miami/vehicles/Boat.o \
	../miami/vehicles/CarGen.o \
	../miami/vehicles/Cranes.o \
	../miami/vehicles/DamageManager.o \
	../miami/vehicles/Door.o \
	../miami/vehicles/Floater.o \
	../miami/vehicles/HandlingMgr.o \
	../miami/vehicles/Heli.o \
	../miami/vehicles/Plane.o \
	../miami/vehicles/Train.o \
	../miami/vehicles/Transmission.o \
	../miami/vehicles/Vehicle.o \
	\
	../miami/weapons/BulletInfo.o \
	../miami/weapons/Explosion.o \
	../miami/weapons/ProjectileInfo.o \
	../miami/weapons/ShotInfo.o \
	../miami/weapons/Weapon.o \
	../miami/weapons/WeaponEffects.o \
	../miami/weapons/WeaponInfo.o \
	\
	../miami/audio/AudioCollision.o \
	../miami/audio/AudioLogic.o \
	../miami/audio/AudioManager.o \
	../miami/audio/AudioScriptObject.o \
	../miami/audio/DMAudio.o \
	../miami/audio/MusicManager.o \
	../miami/audio/PolRadio.o \
	../miami/audio/sampman_miles.o \
	../miami/audio/sampman_oal.o \
	\
	../miami/save/Date.o \
	../miami/save/GenericGameStorage.o \
	../miami/save/MemoryCard.o \
	../miami/save/PCSave.o \
	\
	../miami/extras/debugmenu.o \
	../miami/extras/frontendoption.o \
	../miami/extras/postfx.o \
	../miami/extras/screendroplets.o \
	\
	../vendor/miniLZO/minilzo.o \
	\

# Excluded \
	../miami/extras/custompipes.o \
	../miami/extras/custompipes_d3d9.o \
	../miami/extras/custompipes_gl.o \
	../miami/core/CdStream.o \
	../miami/core/CdStreamPosix.o \
	../miami/extras \
	../miami/extras/GitSHA1.cpp.in \
	../miami/core/AnimViewer.o \

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
-I../miami/animation \
-I../miami/audio \
-I../miami/buildings \
-I../miami/collision \
-I../miami/control \
-I../miami/core \
-I../miami/entities \
-I../miami/extras \
-I../miami/fakerw \
-I../miami/math \
-I../miami/modelinfo \
-I../miami/objects \
-I../miami/peds \
-I../miami/renderer \
-I../miami/rw \
-I../miami/save \
-I../miami/skel \
-I../miami/text \
-I../miami/vehicles \
-I../miami/weapons \
-I../miami/audio/eax \
-I../miami/audio/oal \
-I../miami/extras/shaders \
-I../miami/extras/shaders/obj \
-I../miami/skel/glfw \
-I../miami/skel/win \
\
-I../vendor/librw \
\
-I../vendor/miniLZO

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