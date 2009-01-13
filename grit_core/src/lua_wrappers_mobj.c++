#include <OgreMovableObject.h>
#include <OgreLight.h>
#include <OgreManualObject.h>
#include <OgreEntity.h>
#include <OgreInstancedGeometry.h>
#include <OgreStaticGeometry.h>
#include <OgreCamera.h>

#include "Grit.h"

#include "lua_wrappers_mobj.h"
#include "lua_wrappers_scnmgr.h"
#include "lua_wrappers_primitives.h"
#include "lua_wrappers_material.h"
#include "lua_wrappers_mesh.h"

#include "lua_userdata_dependency_tracker_funcs.h"

// MOVABLE OBJECT ========================================================== {{{


Ogre::MovableObject *check_mobj(lua_State *L,int index)
{
        Ogre::MovableObject *ptr = NULL;
        if (has_tag(L,index,CAM_TAG)) {
                ptr = *static_cast<Ogre::Camera**>(lua_touserdata(L,index));
        } else if (has_tag(L,index,ENTITY_TAG)) {
                ptr = *static_cast<Ogre::Entity**>(lua_touserdata(L,index));
        } else if (has_tag(L,index,LIGHT_TAG)) {
                ptr = *static_cast<Ogre::Light**>(lua_touserdata(L,index));
        }

        if (!ptr) {
                std::string acceptable_types;
                acceptable_types += CAM_TAG ", or ";
                acceptable_types += LIGHT_TAG ", or ";
                acceptable_types += ENTITY_TAG;
                luaL_typerror(L,index,acceptable_types.c_str());
        }
        return ptr;
}


bool push_mobj (lua_State *L, Ogre::MovableObject *mobj)
{
        if (!mobj) {
                lua_pushnil(L);
                return true;
        } else if (dynamic_cast<Ogre::Entity*>(mobj)) {
                push_entity(L,static_cast<Ogre::Entity*>(mobj));
                return true;
        } else if (dynamic_cast<Ogre::Camera*>(mobj)) {
                push_cam(L,static_cast<Ogre::Camera*>(mobj));
                return true;
        } else if (dynamic_cast<Ogre::Light*>(mobj)) {
                push_light(L,static_cast<Ogre::Light*>(mobj));
                return true;
        } else if (dynamic_cast<Ogre::ManualObject*>(mobj)) {
                push_manobj(L,static_cast<Ogre::ManualObject*>(mobj));
                return true;
        }
        return false;
}

static bool mobj_index (lua_State *L, Ogre::MovableObject &self,
                        const std::string &key)
{
        if (key=="name") {
                lua_pushstring(L,self.getName().c_str());
        } else if (key=="movableType") {
                lua_pushstring(L,self.getMovableType().c_str());
        } else if (key == "parentSceneNode") {
                push_node(L,self.getParentSceneNode());
        } else if (key=="renderingDistance") {
                lua_pushnumber(L,self.getRenderingDistance());
        } else if (key=="castShadows") {
                lua_pushboolean(L,self.getCastShadows());
        } else if (key=="renderQueueGroup") {
                lua_pushnumber(L,self.getRenderQueueGroup());
        } else if (key=="visible") {
                lua_pushboolean(L,self.isVisible());
        } else {
                return false;
        }
        return true;
}

static bool mobj_newindex (lua_State *L, Ogre::MovableObject &self,
                           const std::string &key)
{
        if (key=="renderingDistance") {
                lua_Number n = luaL_checknumber(L,3);
                self.setRenderingDistance(n);
        } else if (key=="renderQueueGroup") {
                lua_Number n = check_int(L,3,0,255);
                self.setRenderQueueGroup(n);
        } else if (key=="castShadows") {
                bool b = 0!=lua_toboolean(L,3);
                self.setCastShadows(b);
        } else if (key=="visible") {
                bool b = 0!=lua_toboolean(L,3);
                self.setVisible(b);
        } else {
                return false;
        }
        return true;
}

//}}}


// LIGHT =================================================================== {{{

void push_light (lua_State *L, Ogre::Light *self)
{
        void **ud = static_cast<void**>(lua_newuserdata(L, sizeof(*ud)));
        ud[0] = static_cast<void*> (self);
        luaL_getmetatable(L, LIGHT_TAG);
        lua_setmetatable(L, -2);
        Ogre::SceneManager *sm = self->_getManager();
        scnmgr_maps& maps = grit->getUserDataTables().scnmgrs[sm];
        maps.lights[self].push_back(ud);
}

static int light_gc(lua_State *L)
{
TRY_START
        check_args(L,1);
        GET_UD_MACRO_OFFSET(Ogre::Light,self,1,LIGHT_TAG,0);
        if (self==NULL) return 0;
        Ogre::SceneManager *sm = self->_getManager();
        vec_nullify_remove(grit->getUserDataTables().scnmgrs[sm].lights[self],&self);
        return 0;
TRY_END
}

static int light_destroy (lua_State *L)
{
TRY_START
        check_args(L,1);
        GET_UD_MACRO(Ogre::Light,self,1,LIGHT_TAG);
        Ogre::SceneManager *sm = self._getManager();
        sm->destroyLight(&self);
        map_nullify_remove(grit->getUserDataTables().scnmgrs[sm].lights,&self);
        return 0;
TRY_END
}


static int light_get_diffuse_colour (lua_State *L)
{
TRY_START
        check_args(L,1);
        GET_UD_MACRO(Ogre::Light,self,1,LIGHT_TAG);
        Ogre::ColourValue cv = self.getDiffuseColour();
        lua_pushnumber(L,cv.r);
        lua_pushnumber(L,cv.g);
        lua_pushnumber(L,cv.b);
        return 3;
TRY_END
}

static int light_set_diffuse_colour (lua_State *L)
{
TRY_START
        check_args(L,4);
        GET_UD_MACRO(Ogre::Light,self,1,LIGHT_TAG);
        Ogre::Real r = luaL_checknumber(L,2);
        Ogre::Real g = luaL_checknumber(L,3);
        Ogre::Real b = luaL_checknumber(L,4);
        self.setDiffuseColour(r,g,b);
        return 0;
TRY_END
}


static int light_get_specular_colour (lua_State *L)
{
TRY_START
        check_args(L,1);
        GET_UD_MACRO(Ogre::Light,self,1,LIGHT_TAG);
        Ogre::ColourValue cv = self.getSpecularColour();
        lua_pushnumber(L,cv.r);
        lua_pushnumber(L,cv.g);
        lua_pushnumber(L,cv.b);
        return 3;
TRY_END
}

static int light_set_specular_colour (lua_State *L)
{
TRY_START
        check_args(L,4);
        GET_UD_MACRO(Ogre::Light,self,1,LIGHT_TAG);
        Ogre::Real r = luaL_checknumber(L,2);
        Ogre::Real g = luaL_checknumber(L,3);
        Ogre::Real b = luaL_checknumber(L,4);
        self.setSpecularColour(r,g,b);
        return 0;
TRY_END
}


static int light_get_spotlight_range (lua_State *L)
{
TRY_START
        check_args(L,1);
        GET_UD_MACRO(Ogre::Light,self,1,LIGHT_TAG);
        lua_pushnumber(L,self.getSpotlightInnerAngle().valueDegrees());
        lua_pushnumber(L,self.getSpotlightOuterAngle().valueDegrees());
        lua_pushnumber(L,self.getSpotlightFalloff());
        return 3;
TRY_END
}

static int light_set_spotlight_range (lua_State *L)
{
TRY_START
        check_args(L,4);
        GET_UD_MACRO(Ogre::Light,self,1,LIGHT_TAG);
        Ogre::Real i = luaL_checknumber(L,2);
        Ogre::Real o = luaL_checknumber(L,3);
        Ogre::Real f = luaL_checknumber(L,4);
        self.setSpotlightRange(Ogre::Degree(i),Ogre::Degree(o),f);
        return 0;
TRY_END
}


static int light_get_attenuation (lua_State *L)
{
TRY_START
        check_args(L,1);
        GET_UD_MACRO(Ogre::Light,self,1,LIGHT_TAG);
        lua_pushnumber(L,self.getAttenuationRange());
        lua_pushnumber(L,self.getAttenuationConstant());
        lua_pushnumber(L,self.getAttenuationLinear());
        lua_pushnumber(L,self.getAttenuationQuadric());
        return 4;
TRY_END
}

static int light_set_attenuation (lua_State *L)
{
TRY_START
        check_args(L,5);
        GET_UD_MACRO(Ogre::Light,self,1,LIGHT_TAG);
        Ogre::Real r = luaL_checknumber(L,2);
        Ogre::Real c = luaL_checknumber(L,3);
        Ogre::Real l = luaL_checknumber(L,4);
        Ogre::Real q = luaL_checknumber(L,5);
        self.setAttenuation(r,c,l,q);
        return 0;
TRY_END
}


TOSTRING_ADDR_MACRO(light,Ogre::Light,LIGHT_TAG)

std::string light_type_to_string (lua_State *L, const Ogre::Light::LightTypes t)
{
        switch (t) {
        case Ogre::Light::LT_POINT: return "POINT";
        case Ogre::Light::LT_DIRECTIONAL: return "DIRECTIONAL";
        case Ogre::Light::LT_SPOTLIGHT: return "SPOTLIGHT";
        default:
                my_lua_error(L,"Unrecognised light type.");
        }
        return "";
}

Ogre::Light::LightTypes light_type_from_string (lua_State *L,
                                                const std::string &t)
{
        if (t=="POINT") return Ogre::Light::LT_POINT;
        if (t=="DIRECTIONAL") return Ogre::Light::LT_DIRECTIONAL;
        if (t=="SPOTLIGHT") return Ogre::Light::LT_SPOTLIGHT;
        my_lua_error(L,"Unrecognised light type: "+t);
        return Ogre::Light::LT_POINT; // never gets here
}

static int light_index(lua_State *L)
{
TRY_START
        check_args(L,2);
        GET_UD_MACRO(Ogre::Light,self,1,LIGHT_TAG);
        std::string key = luaL_checkstring(L,2);
        if (key=="destroy") {
                push_cfunction(L,light_destroy);
        } else if (key=="castShadows") {
                lua_pushboolean(L,self.getCastShadows());
        } else if (key=="position") {
                push(L,new Ogre::Vector3(self.getPosition()),VECTOR3_TAG);
        } else if (key=="direction") {
                push(L,new Ogre::Vector3(self.getDirection()),VECTOR3_TAG);
        } else if (key=="getDiffuseColour") {
                push_cfunction(L,light_get_diffuse_colour);
        } else if (key=="setDiffuseColour") {
                push_cfunction(L,light_set_diffuse_colour);
        } else if (key=="getSpecularColour") {
                push_cfunction(L,light_get_specular_colour);
        } else if (key=="setSpecularColour") {
                push_cfunction(L,light_set_specular_colour);
        } else if (key=="getSpotlightRange") {
                push_cfunction(L,light_get_spotlight_range);
        } else if (key=="setSpotlightRange") {
                push_cfunction(L,light_set_spotlight_range);
        } else if (key=="getAttenuation") {
                push_cfunction(L,light_get_attenuation);
        } else if (key=="setAttenuation") {
                push_cfunction(L,light_set_attenuation);
        } else if (key=="powerScale") {
                lua_pushnumber(L, self.getPowerScale());
        } else if (key=="type") {
                lua_pushstring(L,
                               light_type_to_string(L,self.getType()).c_str());
        } else if (!mobj_index(L,self,key)) {
                my_lua_error(L,"Not a valid Light member: "+key);
        }
        return 1;
TRY_END
}

static int light_newindex(lua_State *L)
{
TRY_START
        check_args(L,3);
        GET_UD_MACRO(Ogre::Light,self,1,LIGHT_TAG);
        std::string key = luaL_checkstring(L,2);
        if (key=="powerScale") {
                Ogre::Real v = luaL_checknumber(L,3);
                self.setPowerScale(v);
        } else if (key=="castShadows") {
                bool b = 0!=lua_toboolean(L,3);
                self.setCastShadows(b);
        } else if (key=="position") {
                GET_UD_MACRO(Ogre::Vector3,v,3,VECTOR3_TAG);
                self.setPosition(v);
        } else if (key=="direction") {
                GET_UD_MACRO(Ogre::Vector3,v,3,VECTOR3_TAG);
                self.setDirection(v);
        } else if (key=="type") {
                self.setType
                        (light_type_from_string(L,luaL_checkstring(L,3)));
        } else if (!mobj_newindex(L,self,key)) {
                my_lua_error(L,"Not a valid Light member: "+key);
        }
        return 0;
TRY_END
}

EQ_PTR_MACRO(Ogre::Light,light,LIGHT_TAG)

MT_MACRO_NEWINDEX(light);

//}}}


// MANUAL OBJECT =========================================================== {{{

void push_manobj(lua_State *L, Ogre::ManualObject *man)
{
        void **ud = static_cast<void**>(lua_newuserdata(L, sizeof(*ud)));
        ud[0] = static_cast<void*> (man);
        Ogre::SceneManager *scnmgr = man->_getManager();
        luaL_getmetatable(L, MANOBJ_TAG);
        lua_setmetatable(L, -2);
        grit->getUserDataTables().scnmgrs[scnmgr].manobjs[man].push_back(ud);
}


static int manobj_get_polygon_mode_overrideable (lua_State *L)
{
TRY_START
        check_args(L,2);
        GET_UD_MACRO(Ogre::ManualObject,self,1,MANOBJ_TAG);
        unsigned int n = (unsigned int) check_int(L,2,0,UINT_MAX);
        Ogre::ManualObject::ManualObjectSection *s = self.getSection(n);
        lua_pushboolean(L,s->getPolygonModeOverrideable());
        return 1;
TRY_END
}

static int manobj_set_polygon_mode_overrideable (lua_State *L)
{
TRY_START
        check_args(L,3);
        GET_UD_MACRO(Ogre::ManualObject,self,1,MANOBJ_TAG);
        unsigned int n = (unsigned int) check_int(L,2,0,UINT_MAX);
        bool b = 0!=lua_toboolean(L,3);
        Ogre::ManualObject::ManualObjectSection *s = self.getSection(n);
        s->setPolygonModeOverrideable(b);
        return 0;
TRY_END
}


static int manobj_get_material (lua_State *L)
{
TRY_START
        check_args(L,2);
        GET_UD_MACRO(Ogre::ManualObject,self,1,MANOBJ_TAG);
        unsigned int n = (unsigned int) check_int(L,2,0,UINT_MAX);
        Ogre::ManualObject::ManualObjectSection *s = self.getSection(n);
        push(L,new Ogre::MaterialPtr(s->getMaterial()),MAT_TAG);
        return 1;
TRY_END
}

static int manobj_set_material (lua_State *L)
{
TRY_START
        check_args(L,3);
        GET_UD_MACRO(Ogre::ManualObject,self,1,MANOBJ_TAG);
        unsigned int n = (unsigned int) check_int(L,2,0,UINT_MAX);
        std::string name = luaL_checkstring(L,3);
        Ogre::ManualObject::ManualObjectSection *s = self.getSection(n);
        s->setMaterialName(name);
        return 0;
TRY_END
}


static int manobj_destroy (lua_State *L)
{
TRY_START
        check_args(L,1);
        GET_UD_MACRO(Ogre::ManualObject,self,1,MANOBJ_TAG);
        Ogre::SceneManager *scnmgr = self._getManager();
        scnmgr->destroyManualObject(&self);
        map_nullify_remove(grit->getUserDataTables().scnmgrs[scnmgr].manobjs,
                           &self);
        return 0;
TRY_END
}


TOSTRING_GETNAME_MACRO(manobj,Ogre::ManualObject,.getName(),MANOBJ_TAG)

static int manobj_gc(lua_State *L)
{
TRY_START
        check_args(L,1);
        GET_UD_MACRO_OFFSET(Ogre::ManualObject,ent,1,MANOBJ_TAG,0);
        if (ent==NULL) return 0;
        Ogre::SceneManager *scnmgr = ent->_getManager();
        vec_nullify_remove(
                       grit->getUserDataTables().scnmgrs[scnmgr].manobjs[ent],
                           &ent);
        return 0;
TRY_END
}

static int manobj_index(lua_State *L)
{
TRY_START
        check_args(L,2);
        GET_UD_MACRO(Ogre::ManualObject,self,1,MANOBJ_TAG);
        std::string key = luaL_checkstring(L,2);
        if (key == "numSubSections") {
                lua_pushnumber(L,self.getNumSections());
        } else if (key == "getPolygonModeOverrideable") {
                push_cfunction(L,manobj_get_polygon_mode_overrideable);
        } else if (key == "setPolygonModeOverrideable") {
                push_cfunction(L,manobj_set_polygon_mode_overrideable);
        } else if (key == "getMaterial") {
                push_cfunction(L,manobj_get_material);
        } else if (key == "setMaterial") {
                push_cfunction(L,manobj_set_material);
        } else if (key == "destroy") {
                push_cfunction(L,manobj_destroy);
        } else if (!mobj_index(L,self,key)) {
                my_lua_error(L,"Not a valid ManualObject member: " + key);
        }
        return 1;
TRY_END
}

static int manobj_newindex(lua_State *L)
{
TRY_START
        check_args(L,3);
        GET_UD_MACRO(Ogre::ManualObject,self,1,MANOBJ_TAG);
        std::string key = luaL_checkstring(L,2);
        if (false) {
        } else if (!mobj_newindex(L,self,key)) {
                my_lua_error(L,"Not a valid ManualObject member: " + key);
        }
        return 0;
TRY_END
}

EQ_PTR_MACRO(Ogre::ManualObject,manobj,MANOBJ_TAG)

MT_MACRO_NEWINDEX(manobj);

//}}}


// ENTITY ================================================================== {{{

void push_entity(lua_State *L, Ogre::Entity *ent)
{
        void **ud = static_cast<void**>(lua_newuserdata(L, sizeof(*ud)));
        ud[0] = static_cast<void*> (ent);
        Ogre::SceneManager *scnmgr = ent->_getManager();
        luaL_getmetatable(L, ENTITY_TAG);
        lua_setmetatable(L, -2);
        grit->getUserDataTables().scnmgrs[scnmgr].entities[ent].push_back(ud);
}


static int entity_get_polygon_mode_overrideable (lua_State *L)
{
TRY_START
        check_args(L,2);
        GET_UD_MACRO(Ogre::Entity,self,1,ENTITY_TAG);
        unsigned int n = (unsigned int) check_int(L,2,0,UINT_MAX);
        Ogre::SubEntity *se = self.getSubEntity(n);
        lua_pushboolean(L,se->getPolygonModeOverrideable());
        return 1;
TRY_END
}

static int entity_set_polygon_mode_overrideable (lua_State *L)
{
TRY_START
        check_args(L,3);
        GET_UD_MACRO(Ogre::Entity,self,1,ENTITY_TAG);
        unsigned int n = (unsigned int) check_int(L,2,0,UINT_MAX);
        bool b = 0!=lua_toboolean(L,3);
        Ogre::SubEntity *se = self.getSubEntity(n);
        se->setPolygonModeOverrideable(b);
        return 0;
TRY_END
}


static int entity_get_material (lua_State *L)
{
TRY_START
        check_args(L,2);
        GET_UD_MACRO(Ogre::Entity,self,1,ENTITY_TAG);
        unsigned int n = (unsigned int) check_int(L,2,0,UINT_MAX);
        Ogre::SubEntity *se = self.getSubEntity(n);
        push(L,new Ogre::MaterialPtr(se->getMaterial()),MAT_TAG);
        return 1;
TRY_END
}

static int entity_get_material_name (lua_State *L)
{
TRY_START
        check_args(L,2);
        GET_UD_MACRO(Ogre::Entity,self,1,ENTITY_TAG);
        unsigned int n = (unsigned int) check_int(L,2,0,UINT_MAX);
        Ogre::SubEntity *se = self.getSubEntity(n);
        lua_pushstring(L,se->getMaterialName().c_str());
        return 1;
TRY_END
}

static int entity_set_material (lua_State *L)
{
TRY_START
        check_args(L,3);
        GET_UD_MACRO(Ogre::Entity,self,1,ENTITY_TAG);
        unsigned int n = (unsigned int) check_int(L,2,0,UINT_MAX);
        std::string name = luaL_checkstring(L,3);
        Ogre::SubEntity *se = self.getSubEntity(n);
        se->setMaterialName(name);
        return 0;
TRY_END
}


static int entity_set_custom_parameter (lua_State *L)
{
TRY_START
        check_args(L,7);
        GET_UD_MACRO(Ogre::Entity,self,1,ENTITY_TAG);
        unsigned int n = (unsigned int) check_int(L,2,0,UINT_MAX);
        unsigned int varindex = (unsigned int) check_int(L,3,0,UINT_MAX);
        Ogre::Real v1 = luaL_checknumber(L,4);
        Ogre::Real v2 = luaL_checknumber(L,5);
        Ogre::Real v3 = luaL_checknumber(L,6);
        Ogre::Real v4 = luaL_checknumber(L,7);
        Ogre::SubEntity *se = self.getSubEntity(n);
        se->setCustomParameter(varindex,Ogre::Vector4(v1,v2,v3,v4));
        return 0;
TRY_END
}


static int entity_get_custom_parameter (lua_State *L)
{
TRY_START
        check_args(L,3);
        GET_UD_MACRO(Ogre::Entity,self,1,ENTITY_TAG);
        unsigned int n = (unsigned int) check_int(L,2,0,UINT_MAX);
        unsigned int varindex = (unsigned int) check_int(L,3,0,UINT_MAX);
        Ogre::SubEntity *se = self.getSubEntity(n);
        const Ogre::Vector4 &cp = se->getCustomParameter(varindex);
        lua_pushnumber(L,cp[0]);
        lua_pushnumber(L,cp[1]);
        lua_pushnumber(L,cp[2]);
        lua_pushnumber(L,cp[3]);
        return 4;
TRY_END
}


static int entity_destroy (lua_State *L)
{
TRY_START
        check_args(L,1);
        GET_UD_MACRO(Ogre::Entity,self,1,ENTITY_TAG);
        Ogre::SceneManager *scnmgr = self._getManager();
        scnmgr->destroyEntity(&self);
        map_nullify_remove(grit->getUserDataTables().scnmgrs[scnmgr].entities,
                           &self);
        return 0;
TRY_END
}


TOSTRING_GETNAME_MACRO(entity,Ogre::Entity,.getName(),ENTITY_TAG)

static int entity_gc(lua_State *L)
{
TRY_START
        check_args(L,1);
        GET_UD_MACRO_OFFSET(Ogre::Entity,ent,1,ENTITY_TAG,0);
        if (ent==NULL) return 0;
        Ogre::SceneManager *scnmgr = ent->_getManager();
        vec_nullify_remove(
                       grit->getUserDataTables().scnmgrs[scnmgr].entities[ent],
                           &ent);
        return 0;
TRY_END
}

static int entity_index(lua_State *L)
{
TRY_START
        check_args(L,2);
        GET_UD_MACRO(Ogre::Entity,self,1,ENTITY_TAG);
        std::string key = luaL_checkstring(L,2);
        if (key == "displaySkeleton") {
                lua_pushboolean(L,self.getDisplaySkeleton());
        } else if (key == "numSubEntities") {
                lua_pushnumber(L,self.getNumSubEntities());
        } else if (key == "mesh") {
                push_mesh(L,self.getMesh());
        } else if (key == "getPolygonModeOverrideable") {
                push_cfunction(L,entity_get_polygon_mode_overrideable);
        } else if (key == "setPolygonModeOverrideable") {
                push_cfunction(L,entity_set_polygon_mode_overrideable);
        } else if (key == "getMaterial") {
                push_cfunction(L,entity_get_material);
        } else if (key == "getMaterialName") {
                push_cfunction(L,entity_get_material_name);
        } else if (key == "setMaterial") {
                push_cfunction(L,entity_set_material);
        } else if (key == "setCustomParameter") {
                push_cfunction(L,entity_set_custom_parameter);
        } else if (key == "getCustomParameter") {
                push_cfunction(L,entity_get_custom_parameter);
        } else if (key == "destroy") {
                push_cfunction(L,entity_destroy);
        } else if (!mobj_index(L,self,key)) {
                my_lua_error(L,"Not a readable Entity member: " + key);
        }
        return 1;
TRY_END
}

static int entity_newindex(lua_State *L)
{
TRY_START
        check_args(L,3);
        GET_UD_MACRO(Ogre::Entity,self,1,ENTITY_TAG);
        std::string key = luaL_checkstring(L,2);
        if (key == "displaySkeleton") {
                bool b = 0!=lua_toboolean(L,3);
                self.setDisplaySkeleton(b);
        } else if (!mobj_newindex(L,self,key)) {
                my_lua_error(L,"Not a writeable Entity member: " + key);
        }
        return 0;
TRY_END
}

EQ_PTR_MACRO(Ogre::Entity,entity,ENTITY_TAG)

MT_MACRO_NEWINDEX(entity);

//}}}


// INSTANCED GEOM ========================================================== {{{


void push_instgeo (lua_State *L, Ogre::SceneManager *scnmgr,
                         Ogre::InstancedGeometry *g)
{
        if (!g) {
                lua_pushnil(L);
                return;
        }
        void **ud = static_cast<void**>(lua_newuserdata(L, 2*sizeof(*ud)));
        ud[0] = static_cast<void*> (g);
        ud[1] = static_cast<void*> (scnmgr);
        luaL_getmetatable(L, INSTGEO_TAG);
        lua_setmetatable(L, -2);
        APP_ASSERT(scnmgr!=NULL);
        APP_ASSERT(g!=NULL);
        APP_ASSERT(ud!=NULL);
        grit->getUserDataTables().scnmgrs[scnmgr].instgeoms[g].push_back(ud);
}


static int instgeo_reset (lua_State *L)
{
TRY_START
        check_args(L,1);
        GET_UD_MACRO(Ogre::InstancedGeometry,self,1,INSTGEO_TAG);
        self.reset();
        return 0;
TRY_END
}

static int instgeo_build (lua_State *L)
{
TRY_START
        check_args(L,1);
        GET_UD_MACRO(Ogre::InstancedGeometry,self,1,INSTGEO_TAG);
        self.build();
        return 0;
TRY_END
}

static int instgeo_add_entity (lua_State *L)
{
TRY_START
        check_args(L,5);
        GET_UD_MACRO(Ogre::InstancedGeometry,self,1,INSTGEO_TAG);
        GET_UD_MACRO(Ogre::Entity,ent,2,ENTITY_TAG);
        GET_UD_MACRO(Ogre::Vector3,pos,3,VECTOR3_TAG);
        GET_UD_MACRO(Ogre::Quaternion,q,4,QUAT_TAG);
        GET_UD_MACRO(Ogre::Vector3,scale,5,VECTOR3_TAG);

        self.addEntity(&ent,pos,q,scale);
        return 0;
TRY_END
}

static int instgeo_destroy (lua_State *L)
{
TRY_START
        check_args(L,1);
        GET_UD_MACRO(Ogre::InstancedGeometry,self,1,INSTGEO_TAG);
        GET_UD_MACRO_OFFSET(Ogre::SceneManager,scnmgr,1,INSTGEO_TAG,1);
        scnmgr->destroyInstancedGeometry(&self);
        map_nullify_remove(grit->getUserDataTables().scnmgrs[scnmgr].instgeoms,
                           &self);

        return 0;
TRY_END
}



TOSTRING_GETNAME_MACRO(instgeo,Ogre::InstancedGeometry,.getName(),INSTGEO_TAG)

static int instgeo_gc(lua_State *L)
{
TRY_START
        check_args(L,1);
        GET_UD_MACRO_OFFSET(Ogre::InstancedGeometry,self,1,INSTGEO_TAG,0);
        if (self==NULL) return 0;
        GET_UD_MACRO_OFFSET(Ogre::SceneManager,scnmgr,1,INSTGEO_TAG,1);
        vec_nullify_remove(
                     grit->getUserDataTables().scnmgrs[scnmgr].instgeoms[self],
                           &self);
        return 0;
TRY_END
}

static int instgeo_index(lua_State *L)
{
TRY_START
        check_args(L,2);
        GET_UD_MACRO(Ogre::InstancedGeometry,self,1,INSTGEO_TAG);
        std::string key  = luaL_checkstring(L,2);
        if (key=="reset") {
                push_cfunction(L,instgeo_reset);
        } else if (key=="build") {
                push_cfunction(L,instgeo_build);
        } else if (key=="renderingDistance") {
                lua_pushnumber(L,self.getRenderingDistance());
        } else if (key=="squaredRenderingDistance") {
                lua_pushnumber(L,self.getSquaredRenderingDistance());
        } else if (key=="regionDimensions") {
                push(L,new Ogre::Vector3(self.getBatchInstanceDimensions()),
                       VECTOR3_TAG);
        } else if (key=="origin") {
                push(L,new Ogre::Vector3(self.getOrigin()),VECTOR3_TAG);

        } else if (key=="addEntity") {
                push_cfunction(L,instgeo_add_entity);
        } else if (key=="name") {
                lua_pushstring(L,self.getName().c_str());
        } else if (key=="destroy") {
                push_cfunction(L,instgeo_destroy);
        } else {
                std::string s("Not a valid Instanced Geometry member: ");
                my_lua_error(L,s+key);
        }
        return 1;
TRY_END
}

static int instgeo_newindex(lua_State *L)
{
TRY_START
        check_args(L,3);
        GET_UD_MACRO(Ogre::InstancedGeometry,self,1,INSTGEO_TAG);
        std::string key  = luaL_checkstring(L,2);
        if (key=="renderingDistance") {
                lua_Number n = luaL_checknumber(L,3);
                self.setRenderingDistance(n);
        } else if (key=="regionDimensions") {
                GET_UD_MACRO(Ogre::Vector3,v,3,VECTOR3_TAG);
                self.setBatchInstanceDimensions(v);
        } else if (key=="origin") {
                GET_UD_MACRO(Ogre::Vector3,v,3,VECTOR3_TAG);
                self.setOrigin(v);
        } else {
                std::string s("Not a valid Instanced Geometry member: ");
                my_lua_error(L,s+key);
        }
        return 1;
TRY_END
}

EQ_PTR_MACRO(Ogre::InstancedGeometry,instgeo,INSTGEO_TAG)


MT_MACRO_NEWINDEX(instgeo);

//}}}


// STATIC GEOM ============================================================= {{{

void push_statgeo (lua_State *L, Ogre::SceneManager *scnmgr,
                         Ogre::StaticGeometry *g)
{
        void **ud = static_cast<void**>(lua_newuserdata(L, 2*sizeof(*ud)));
        ud[0] = static_cast<void*> (g);
        ud[1] = static_cast<void*> (scnmgr);
        luaL_getmetatable(L, STATGEO_TAG);
        lua_setmetatable(L, -2);
        APP_ASSERT(scnmgr!=NULL);
        APP_ASSERT(g!=NULL);
        APP_ASSERT(ud!=NULL);
        grit->getUserDataTables().scnmgrs[scnmgr].statgeoms[g].push_back(ud);
}


static int statgeo_reset (lua_State *L)
{
TRY_START
        check_args(L,1);
        GET_UD_MACRO(Ogre::StaticGeometry,self,1,STATGEO_TAG);
        self.reset();
        return 0;
TRY_END
}

static int statgeo_build (lua_State *L)
{
TRY_START
        check_args(L,1);
        GET_UD_MACRO(Ogre::StaticGeometry,self,1,STATGEO_TAG);
        self.build();
        return 0;
TRY_END
}

static int statgeo_add_entity (lua_State *L)
{
TRY_START
        check_args(L,5);
        GET_UD_MACRO(Ogre::StaticGeometry,self,1,STATGEO_TAG);
        GET_UD_MACRO(Ogre::Entity,ent,2,ENTITY_TAG);
        GET_UD_MACRO(Ogre::Vector3,pos,3,VECTOR3_TAG);
        GET_UD_MACRO(Ogre::Quaternion,q,4,QUAT_TAG);
        GET_UD_MACRO(Ogre::Vector3,scale,5,VECTOR3_TAG);

        self.addEntity(&ent,pos,q,scale);
        return 0;
TRY_END
}

static int statgeo_destroy (lua_State *L)
{
TRY_START
        check_args(L,1);
        GET_UD_MACRO(Ogre::StaticGeometry,self,1,STATGEO_TAG);
        GET_UD_MACRO_OFFSET(Ogre::SceneManager,scnmgr,1,STATGEO_TAG,1);
        scnmgr->destroyStaticGeometry(&self);
        map_nullify_remove(grit->getUserDataTables().scnmgrs[scnmgr].statgeoms,
                           &self);

        return 0;
TRY_END
}



TOSTRING_GETNAME_MACRO(statgeo,Ogre::StaticGeometry,.getName(),STATGEO_TAG)

static int statgeo_gc(lua_State *L)
{
TRY_START
        check_args(L,1);
        GET_UD_MACRO_OFFSET(Ogre::StaticGeometry,self,1,STATGEO_TAG,0);
        if (self==NULL) return 0;
        GET_UD_MACRO_OFFSET(Ogre::SceneManager,scnmgr,1,STATGEO_TAG,1);
        vec_nullify_remove(
                     grit->getUserDataTables().scnmgrs[scnmgr].statgeoms[self],
                           &self);
        return 0;
TRY_END
}

static int statgeo_index(lua_State *L)
{
TRY_START
        check_args(L,2);
        GET_UD_MACRO(Ogre::StaticGeometry,self,1,STATGEO_TAG);
        std::string key  = luaL_checkstring(L,2);
        if (key=="reset") {
                push_cfunction(L,statgeo_reset);
        } else if (key=="build") {
                push_cfunction(L,statgeo_build);
        } else if (key=="renderingDistance") {
                lua_pushnumber(L,self.getRenderingDistance());
        } else if (key=="squaredRenderingDistance") {
                lua_pushnumber(L,self.getSquaredRenderingDistance());
        } else if (key=="regionDimensions") {
                push(L,new Ogre::Vector3(self.getRegionDimensions()),
                       VECTOR3_TAG);
        } else if (key=="origin") {
                push(L,new Ogre::Vector3(self.getOrigin()),VECTOR3_TAG);

        } else if (key=="addEntity") {
                push_cfunction(L,statgeo_add_entity);
        } else if (key=="name") {
                lua_pushstring(L,self.getName().c_str());
        } else if (key=="destroy") {
                push_cfunction(L,statgeo_destroy);
        } else {
                std::string s("Not a valid Static Geometry member: ");
                my_lua_error(L,s+key);
        }
        return 1;
TRY_END
}

static int statgeo_newindex(lua_State *L)
{
TRY_START
        check_args(L,3);
        GET_UD_MACRO(Ogre::StaticGeometry,self,1,STATGEO_TAG);
        std::string key  = luaL_checkstring(L,2);
        if (key=="renderingDistance") {
                lua_Number n = luaL_checknumber(L,3);
                self.setRenderingDistance(n);
        } else if (key=="regionDimensions") {
                GET_UD_MACRO(Ogre::Vector3,v,3,VECTOR3_TAG);
                self.setRegionDimensions(v);
        } else if (key=="origin") {
                GET_UD_MACRO(Ogre::Vector3,v,3,VECTOR3_TAG);
                self.setOrigin(v);
        } else {
                std::string s("Not a valid Static Geometry member: ");
                my_lua_error(L,s+key);
        }
        return 1;
TRY_END
}

EQ_PTR_MACRO(Ogre::StaticGeometry,statgeo,STATGEO_TAG)

MT_MACRO_NEWINDEX(statgeo);

//}}}


// CAMERA ================================================================== {{{



void push_cam (lua_State *L, Ogre::Camera *cam)
{
        void **ud = static_cast<void**>(lua_newuserdata(L, sizeof(*ud)));
        ud[0] = static_cast<void*> (cam);
        Ogre::SceneManager *scnmgr = cam->getSceneManager();
        luaL_getmetatable(L, CAM_TAG);
        lua_setmetatable(L, -2);
        grit->getUserDataTables().scnmgrs[scnmgr].cameras[cam].push_back(ud);
}

static int cam_destroy (lua_State *L)
{
TRY_START
        check_args(L,1);
        GET_UD_MACRO(Ogre::Camera,self,1,CAM_TAG);
        Ogre::SceneManager *scnmgr = self.getSceneManager();
        scnmgr->destroyCamera(&self);
        map_nullify_remove(grit->getUserDataTables().scnmgrs[scnmgr].cameras,
                           &self);
        return 0;
TRY_END
}



TOSTRING_MACRO(cam,Ogre::Camera,CAM_TAG)
//TOSTRING_GETNAME_MACRO(cam,Ogre::Camera,.getName(),CAM_TAG)

static int cam_gc(lua_State *L)
{
TRY_START
        check_args(L,1);
        GET_UD_MACRO_OFFSET(Ogre::Camera,self,1,CAM_TAG,0);
        if (self==NULL) return 0;
        Ogre::SceneManager *scnmgr = self->getSceneManager();
        vec_nullify_remove(
                       grit->getUserDataTables().scnmgrs[scnmgr].cameras[self],
                           &self);
        return 0;
TRY_END
}

static int cam_index(lua_State *L)
{
TRY_START
        check_args(L,2);
        GET_UD_MACRO(Ogre::Camera,self,1,CAM_TAG);
        std::string key  = luaL_checkstring(L,2);
        if (key=="position") {
                push(L,new Ogre::Vector3(self.getPosition()),VECTOR3_TAG);
        } else if (key=="orientation") {
                push(L,new Ogre::Quaternion(self.getOrientation()),QUAT_TAG);
        } else if (key=="derivedPosition") {
                push(L,new Ogre::Vector3(self.getDerivedPosition()),
                       VECTOR3_TAG);
        } else if (key=="derivedOrientation") {
                push(L,new Ogre::Quaternion(self.getDerivedOrientation()),
                       QUAT_TAG);
        } else if (key=="polygonMode") {
                std::string s = polygon_mode_to_string(L,self.getPolygonMode());
                lua_pushstring(L,s.c_str());
        } else if (key=="focalLength") {
                lua_pushnumber(L,self.getFocalLength());
        } else if (key=="frustumOffsetX") {
                lua_pushnumber(L,self.getFrustumOffset().x);
        } else if (key=="frustumOffsetY") {
                lua_pushnumber(L,self.getFrustumOffset().y);
        } else if (key=="autoAspectRatio") {
                lua_pushboolean(L,self.getAutoAspectRatio());
        } else if (key=="FOV") {
                lua_pushnumber(L,self.getFOVy().valueDegrees());
        } else if (key=="farClipDistance") {
                lua_pushnumber(L,self.getFarClipDistance());
        } else if (key=="nearClipDistance") {
                lua_pushnumber(L,self.getNearClipDistance());
        } else if (key=="useRenderingDistance") {
                lua_pushboolean(L,self.getUseRenderingDistance());
        } else if (key=="destroy") {
                push_cfunction(L,cam_destroy);
        } else if (!mobj_index(L,self,key)) {
                my_lua_error(L,"Not a valid Camera member: " + key);
        }
        return 1;
TRY_END
}

static int cam_newindex(lua_State *L)
{
TRY_START
        check_args(L,3);
        GET_UD_MACRO(Ogre::Camera,self,1,CAM_TAG);
        std::string key  = luaL_checkstring(L,2);
        if (key=="position") {
                GET_UD_MACRO(Ogre::Vector3,v,3,VECTOR3_TAG);
                self.setPosition(v);
        } else if (key=="orientation") {
                GET_UD_MACRO(Ogre::Quaternion,q,3,QUAT_TAG);
                self.setOrientation(q);
        } else if (key=="polygonMode") {
                std::string s = luaL_checkstring(L,3);
                self.setPolygonMode(polygon_mode_from_string(L,s));
        } else if (key=="focalLength") {
                lua_Number n = luaL_checknumber(L,3);
                self.setFocalLength(n);
        } else if (key=="frustumOffsetX") {
                Ogre::Vector2 v = self.getFrustumOffset();
                v.x = luaL_checknumber(L,3);
                self.setFrustumOffset(v);
        } else if (key=="frustumOffsetY") {
                Ogre::Vector2 v = self.getFrustumOffset();
                v.y = luaL_checknumber(L,3);
                self.setFrustumOffset(v);
        } else if (key=="autoAspectRatio") {
                bool v = 0!=lua_toboolean(L,3);
                self.setAutoAspectRatio(v);
        } else if (key=="FOV") {
                lua_Number n = luaL_checknumber(L,3);
                if (n<=0) {
                        my_lua_error(L,"FOV must be greater than 0.");
                } else if (n>=180) {
                        my_lua_error(L,"FOV must be less than 180.");
                }
                self.setFOVy(Ogre::Degree(n));
        } else if (key=="farClipDistance") {
                lua_Number n = luaL_checknumber(L,3);
                self.setFarClipDistance(n);
        } else if (key=="nearClipDistance") {
                lua_Number n = luaL_checknumber(L,3);
                self.setNearClipDistance(n);
        } else if (key=="useRenderingDistance") {
                bool b = 0!=lua_toboolean(L,3);
                self.setUseRenderingDistance(b);
        } else {
                my_lua_error(L,"Not a valid Camera member: " + key);
        }
        return 0;
TRY_END
}

EQ_PTR_MACRO(Ogre::Camera,cam,CAM_TAG)

MT_MACRO_NEWINDEX(cam);

//}}}



// vim: shiftwidth=8:tabstop=8:expandtab