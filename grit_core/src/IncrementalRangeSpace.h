template <typename T> class IncrementalRangeSpace;

#ifndef INCREMENTALRANGESPACE_H
#define INCREMENTALRANGESPACE_H

#include <map>
#include <vector>
#include <iostream>
#include <math.h>


#include "app_error.h"

template <typename T>
class IncrementalRangeSpace {

    public:

        typedef std::set<T*> CargoMap;
        typedef std::set<T*> CargoSet;

        IncrementalRangeSpace() : mLast(NULL) { }

        virtual ~IncrementalRangeSpace() { }

        virtual void add (T *o)
        {
                // if it's already in there, this is a no-op
                mEverything.insert(o);
        }

        virtual const CargoMap& getPresent (const Ogre::Vector3 &cam,
                                            const Ogre::Real factor,
                                            const unsigned int num,
                                            CargoSet &killed)
        {
                if (num==0) return mWorkingSet;
                typename CargoSet::iterator it = mEverything.find(mLast),
                                   end = mEverything.end();
                if (it == end) {
                        it = mEverything.begin(); 
                        if (it == end) {
                               return mWorkingSet; // empty list
                        }
                }

                // iterate from this point for a while
                T *o;
                for (unsigned int i=0 ; i<num ; ++i) {
                        ++it;
                        if (it==end) {
                                it = mEverything.begin();
                        }
                        o = *it;
                        if (process(o, cam, factor)) {
                                mWorkingSet.insert(o);
                        } else {
                                if (mWorkingSet.erase(o))
                                        killed.insert(o);
                        }
                        if (o==mLast) return mWorkingSet; // we already looped
                }

                mLast = o;

                return mWorkingSet;
        }

        // no-op if o was not in the rangespace somewhere
        virtual void remove (T *o)
        {
                // if it's not in there, these are no-ops
                mWorkingSet.erase(o);
                mEverything.erase(o);
                if (mLast == o) {
                        mLast = NULL;
                }
        }

        virtual void clear (void)
        {
                // if it's not in there, these are no-ops
                mWorkingSet.clear();
                mEverything.clear();
                mLast = NULL;
        }

        std::ostream& toStream (std::ostream& o)
        {
                return o;
        }

    protected:

        virtual bool process (T* o, const Ogre::Vector3 &cam,
                              const Ogre::Real factor)
        {
                if (!o->isInScene()) return false;
                if (!o->getVisible() ) return false;

                Ogre::Real rd = o->getRenderingDistance();

                if (rd==0) return true;

                // now the distance test
                Ogre::SceneNode *n = o->getParentSceneNode();
                if (n==NULL) return false;
                const Ogre::Vector3 &o_pos = n->_getDerivedPosition();

                Ogre::Real sqd = (o_pos - cam).squaredLength();
                if (sqd > Ogre::Math::Sqr(factor*rd))
                        return false;

/* this in general won't work because o may not be loaded yet
                Ogre::Real sqd = (o_pos - cam).squaredLength();
                if (sqd > Ogre::Math::Sqr(factor*(rd+o->getBoundingRadius())))
                        return false;
*/

                return true;
        }

        T *mLast;
        CargoSet mEverything;
        CargoMap mWorkingSet;
};

template <typename T>
std::ostream& operator<<(std::ostream& o, IncrementalRangeSpace<T>& rs)
{
        return rs.toStream(o);
}


#endif

// vim: shiftwidth=8:tabstop=8:expandtab