/* Copyright (c) David Cunningham and the Grit Game Engine project 2010
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "../main.h"
#include "../ExternalTable.h"
#include "../LuaPtr.h"
#include "../path_util.h"

#include "audio.h"
#include "lua_wrappers_audio.h"

#define AUDIOSOURCE_TAG "Grit/AudioSource"

void push_audiosource (lua_State *L, const AudioSourcePtr &self)
{
    if (self.isNull())
        lua_pushnil(L);
    else
        push(L,new AudioSourcePtr(self),AUDIOSOURCE_TAG);
}

GC_MACRO(AudioSourcePtr,audiosource,AUDIOSOURCE_TAG)

static int audiosource_play (lua_State *L)
{
TRY_START
    check_args(L,1);
    GET_UD_MACRO(AudioSourcePtr,self,1,AUDIOSOURCE_TAG);
    self->play();
    return 0;
TRY_END
}

static int audiosource_pause (lua_State *L)
{
TRY_START
    check_args(L,1);
    GET_UD_MACRO(AudioSourcePtr,self,1,AUDIOSOURCE_TAG);
    self->pause();
    return 0;
TRY_END
}

static int audiosource_stop (lua_State *L)
{
TRY_START
    check_args(L,1);
    GET_UD_MACRO(AudioSourcePtr,self,1,AUDIOSOURCE_TAG);
    self->stop();
    return 0;
TRY_END
}

static int audiosource_destroy (lua_State *L)
{
TRY_START
    check_args(L,1);
    GET_UD_MACRO(AudioSourcePtr,self,1,AUDIOSOURCE_TAG);
    self->destroy();
    return 0;
TRY_END
}

TOSTRING_SMART_PTR_MACRO (audiosource,AudioSourcePtr,AUDIOSOURCE_TAG)

static int audiosource_index (lua_State *L)
{
TRY_START
    check_args(L,2);
    GET_UD_MACRO(AudioSourcePtr,self,1,AUDIOSOURCE_TAG);
    const char *key = luaL_checkstring(L,2);
    if (!::strcmp(key,"position")) {
        push_v3(L, self->getPosition());
    } else if (!::strcmp(key,"velocity")) {
        push_v3(L, self->getVelocity());
    } else if (!::strcmp(key,"looping")) {
        lua_pushboolean(L, self->getLoop());
    } else if (!::strcmp(key,"playing")) {
        lua_pushboolean(L, self->playing());
	} else if (!::strcmp(key,"play")) {
		push_cfunction(L,audiosource_play);
	} else if (!::strcmp(key,"pause")) {
		push_cfunction(L,audiosource_pause);
	} else if (!::strcmp(key,"stop")) {
		push_cfunction(L,audiosource_stop);
    } else if (!::strcmp(key,"destroy")) {
        push_cfunction(L,audiosource_destroy);
    } else {
        my_lua_error(L,"Not a readable AudioSource member: "+std::string(key));
    }
    return 1;
TRY_END
}


static int audiosource_newindex (lua_State *L)
{
TRY_START
    check_args(L,3);
    GET_UD_MACRO(AudioSourcePtr,self,1,AUDIOSOURCE_TAG);
    const char *key = luaL_checkstring(L,2);
    if (!::strcmp(key,"position")) {
        Vector3 v = check_v3(L,3);
        self->setPosition(v);
    } else if (!::strcmp(key,"velocity")) {
        Vector3 v = check_v3(L,3);
		self->setVelocity(v);
	} else if (!::strcmp(key,"pitch")) {
		float f = check_float(L, 3);
		self->setPitch(f);
	} else if (!::strcmp(key,"volume")) {
		float f = check_float(L, 3);
		self->setVolume(f);
    } else if (!::strcmp(key,"looping")) {
        bool b = check_bool(L,3);
		self->setLoop(b);
    } else {
        my_lua_error(L,"Not a writable AudioSource member: "+std::string(key));
    }
    return 0;
TRY_END
}

EQ_MACRO(AudioSourcePtr,audiosource,AUDIOSOURCE_TAG)

MT_MACRO_NEWINDEX(audiosource);

static int global_audio_source_make (lua_State *L)
{
TRY_START
	check_args(L, 1);
	push_audiosource(L, AudioSource::make(check_string(L, 1)));
	return 1;
TRY_END
}

static int global_audio_play (lua_State *L)
{
TRY_START
	check_args(L, 2);
	audio_play(check_string(L, 1), check_v3(L, 2));
	return 0;
TRY_END
}

static int global_audio_set_listener (lua_State *L)
{
TRY_START
	check_args(L, 3);
	audio_set_listener(check_v3(L, 1), check_v3(L, 2), check_quat(L, 3));
	return 0;
TRY_END
}

static int global_audio_update (lua_State *L)
{
TRY_START
	check_args(L, 0);
	audio_update();
	return 0;
TRY_END
}

static const luaL_reg global[] = {
	{"audio_source_make",global_audio_source_make},
	{"audio_play",global_audio_play},
	{"audio_set_listener",global_audio_set_listener},
	{"audio_update",global_audio_update},
	{NULL,NULL}
};

void audio_lua_init (lua_State *L)
{
#define ADD_MT_MACRO(name,tag) do {\
	luaL_newmetatable(L, tag); \
	luaL_register(L, NULL, name##_meta_table); \
	lua_pop(L,1); } while(0)

	ADD_MT_MACRO(audiosource,AUDIOSOURCE_TAG);

	luaL_register(L, "_G", global);
}
