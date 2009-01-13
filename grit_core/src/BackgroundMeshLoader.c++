
#include <OgrePrerequisites.h> // for OGRE_THREAD_SUPPORT

#include <OgreRoot.h>
#include <OgreRenderSystem.h>
#include <OgreMeshManager.h>
#include <OgreMesh.h>
#include <OgreSubMesh.h>
#include <OgreMaterialManager.h>
#include <OgreMaterial.h>

#include "sleep.h"
#include "BackgroundMeshLoader.h"


BackgroundMeshLoader::BackgroundMeshLoader (void)
      : mThread(NULL), mCurrent(NULL), mQuit(false), mAllowance(0)
{
        OGRE_LOCK_AUTO_MUTEX;

        mThread = new boost::thread(boost::ref(*this));
}

BackgroundMeshLoader::~BackgroundMeshLoader (void)
{
        shutdown();
}

void BackgroundMeshLoader::shutdown (void)
{
        // make sure a later destruction of this object doesn't touch ogre.
        if (mQuit) return;
        {
                OGRE_LOCK_AUTO_MUTEX;
                mQuit = true;
                OGRE_THREAD_NOTIFY_ONE(mCVar);
        }
        mThread->join();
        handleBastards();
        mDeathRow.clear();
        mDemands.clear();
        delete mThread;
}

void BackgroundMeshLoader::add (Demand *d)
{
        OGRE_LOCK_AUTO_MUTEX;
/*
        std::stringstream ss;
        ss << "adding demand: \""<<d->mMeshName<<"\" {";
        typedef std::set<Ogre::String>::const_iterator I;
        I begin = d->mMatNames.begin();
        I end = d->mMatNames.end();
        for (I i=begin ; i!=end ; ++i) {
                ss << (i==begin?"":", ")<<"\""<<*i<<"\"";
        }
        ss << "}";
        APP_VERBOSE(ss.str());
*/
        mDemands.insert(d);
        OGRE_THREAD_NOTIFY_ONE(mCVar);
}

void BackgroundMeshLoader::remove (Demand *d)
{
        OGRE_LOCK_AUTO_MUTEX;
        mDemands.erase(d);
        if (mCurrent == d)
                mCurrent = NULL;
}               

void BackgroundMeshLoader::handleBastards (void)
{
        ResourcePtrSet s;
        {
                OGRE_LOCK_AUTO_MUTEX;
                if (mBastards.size()==0) return;
                s = mBastards;
                mBastards.clear();
        }

        finishedWith(s);
}


void BackgroundMeshLoader::operator() (void)
{
        //APP_VERBOSE("BackgroundMeshLoader: thread started");
        ResourcePtrs pending;
        while (!mQuit) {
                {
                        OGRE_LOCK_AUTO_MUTEX;
                        if (mCurrent) {
                                // Usual case:
                                // demand wasn't retracted while we were
                                // processing it
                                mCurrent->mProcessed = true;
                                // cache in d to suppress compiler error
                                Demand *d = mCurrent;
                                mDemands.erase(d);
                                mCurrent = NULL;
                        } else {
                                // demand was retracted, and we actually
                                // loaded stuff
                                for (ResourcePtrs::const_iterator
                                        i=pending.begin(), i_=pending.end() ;
                                                                  i!=i_ ; ++i) {
                                        mBastards.insert(*i);
                                }
                               //asynchronously call sm.finishedWith(resource);
                        }
                        pending.clear();
                        if (mAllowance <= 0 || !nearestDemand(mCurrent)) {
                                OGRE_THREAD_WAIT(mCVar,OGRE_AUTO_MUTEX_NAME)
                                continue;        
                        }       
                        pending = mCurrent->rPtrs;
                }
                //APP_VERBOSE("BackgroundMeshLoader: loading: "+name);
                for (ResourcePtrs::iterator i=pending.begin(),
                                            i_=pending.end() ; i!=i_ ; ++i) {
                        Ogre::ResourcePtr &rp = *i;
                        if (!rp->isPrepared()) {
                                rp->prepare();
                                mAllowance--;
                        }
                }

                //mysleep(100000);
        }
        //APP_VERBOSE("BackgroundMeshLoader: thread terminated");
}


bool BackgroundMeshLoader::nearestDemand (Demand * volatile &return_demand)
{

        Ogre::Real closest_dist = 0;
        bool found = false;

        for (Demands::const_iterator i=mDemands.begin(),
                                     i_=mDemands.end()  ; i!=i_ ; ++i) {
                Demand *d = *i;      
                
                Ogre::Real this_dist = (*i)->mDist;
                
                if (!found || this_dist<closest_dist) {
                        found = true;
                        closest_dist = this_dist;
                        return_demand = d;
                }       

        }       

        return found;

}

void BackgroundMeshLoader::setAllowance (Ogre::Real m)
{
        OGRE_LOCK_AUTO_MUTEX
        mAllowance = std::max(mAllowance+m, m);
        OGRE_THREAD_NOTIFY_ONE(mCVar);
}


// unloading of meshes /////////////////////////////////////////////////////////

template<typename T>
static inline bool is_unused (const Ogre::SharedPtr<T>& r)
{
        // if resource only used here, unload it
        return r.useCount() <=
             Ogre::ResourceGroupManager::RESOURCE_SYSTEM_NUM_REFERENCE_COUNTS+1;
}

void BackgroundMeshLoader::finishedWith (const ResourcePtrSet &s) {
        typedef ResourcePtrSet::const_iterator I;
        for (I i=s.begin(), i_=s.end() ; i!=i_ ; ++i) {
                finishedWith(*i);
        }
}

void BackgroundMeshLoader::finishedWith (const Ogre::ResourcePtr &rp)
{
        if (mQuit) return;
        if (rp.isNull()) return;
        if (!is_unused(rp)) return;
        mDeathRow.push(rp);
}

void BackgroundMeshLoader::finishedWith (const Ogre::MaterialPtr &mat)
{
        if (mQuit) return;
        if (mat.isNull()) return;
        if (!is_unused(mat)) return;
        mat->unload();
}

bool BackgroundMeshLoader::isGPUOversubscribed () const
{
        Ogre::TextureManager &tm = Ogre::TextureManager::getSingleton();
        Ogre::MeshManager &mm = Ogre::MeshManager::getSingleton();

        size_t budget = tm.getMemoryBudget() + mm.getMemoryBudget();

        size_t usage = tm.getMemoryUsage() + mm.getMemoryUsage();

        return usage >= budget;
}

void BackgroundMeshLoader::checkGPUUsage ()
{
        Ogre::TextureManager &tm = Ogre::TextureManager::getSingleton();
        Ogre::MeshManager &mm = Ogre::MeshManager::getSingleton();

        size_t budget = tm.getMemoryBudget() + mm.getMemoryBudget();

        while (true) {

                size_t usage = tm.getMemoryUsage() + mm.getMemoryUsage();

                if (usage < budget || mDeathRow.size() == 0) break;

                Ogre::ResourcePtr r = mDeathRow.pop();

                if (is_unused(r))
                        r->unload();
        }

}

template<>
BackgroundMeshLoader *Ogre::Singleton<BackgroundMeshLoader>::ms_Singleton =NULL;


// vim: shiftwidth=8:tabstop=8:expandtab