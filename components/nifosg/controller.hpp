#ifndef COMPONENTS_NIFOSG_CONTROLLER_H
#define COMPONENTS_NIFOSG_CONTROLLER_H

#include <components/nif/niffile.hpp>
#include <components/nif/nifkey.hpp>
#include <components/nif/controller.hpp>
#include <components/nif/data.hpp>

#include <components/nifcache/nifcache.hpp>

#include <boost/shared_ptr.hpp>

#include <set> //UVController

// FlipController
#include <osg/Image>
#include <osg/ref_ptr>

#include <osg/Timer>
#include <osg/StateSet>
#include <osg/NodeCallback>
#include <osg/Drawable>


namespace osg
{
    class Node;
    class StateSet;
}

namespace osgParticle
{
    class Emitter;
}

namespace osgAnimation
{
    class MorphGeometry;
}

namespace NifOsg
{

    // FIXME: Should not be here. We might also want to use this for non-NIF model formats
    class ValueInterpolator
    {
    protected:
        template <typename T>
        T interpKey (const std::map< float, Nif::KeyT<T> >& keys, float time, T defaultValue = T()) const
        {
            if (keys.size() == 0)
                return defaultValue;

            if(time <= keys.begin()->first)
                return keys.begin()->second.mValue;

            typename std::map< float, Nif::KeyT<T> >::const_iterator it = keys.lower_bound(time);
            if (it != keys.end())
            {
                float aTime = it->first;
                const Nif::KeyT<T>* aKey = &it->second;

                assert (it != keys.begin()); // Shouldn't happen, was checked at beginning of this function

                typename std::map< float, Nif::KeyT<T> >::const_iterator last = --it;
                float aLastTime = last->first;
                const Nif::KeyT<T>* aLastKey = &last->second;

                float a = (time - aLastTime) / (aTime - aLastTime);
                return aLastKey->mValue + ((aKey->mValue - aLastKey->mValue) * a);
            }
            else
                return keys.rbegin()->second.mValue;
        }
    };

    // FIXME: Should not be here. We might also want to use this for non-NIF model formats
    class ControllerFunction
    {
    private:
        float mFrequency;
        float mPhase;
        float mStartTime;
        bool mDeltaInput;
        float mDeltaCount;
    public:
        float mStopTime;

    public:
        ControllerFunction(const Nif::Controller *ctrl, bool deltaInput);

        float calculate(float value);
    };
    typedef ControllerFunction DefaultFunction;

    class ControllerSource
    {
    public:
        virtual float getValue(osg::NodeVisitor* nv) = 0;
    };

    class FrameTimeSource : public ControllerSource
    {
    public:
        FrameTimeSource();
        virtual float getValue(osg::NodeVisitor* nv);
    private:
        double mLastTime;
    };

    class Controller
    {
    public:
        Controller();

        bool hasInput() const;

        float getInputValue(osg::NodeVisitor* nv);

        boost::shared_ptr<ControllerSource> mSource;

        // The source value gets passed through this function before it's passed on to the DestValue.
        boost::shared_ptr<ControllerFunction> mFunction;
    };

    class GeomMorpherController : public osg::Drawable::UpdateCallback, public Controller, public ValueInterpolator
    {
    public:
        GeomMorpherController(const Nif::NiMorphData* data);
        GeomMorpherController();
        GeomMorpherController(const GeomMorpherController& copy, const osg::CopyOp& copyop);

        META_Object(NifOsg, GeomMorpherController)

        virtual void update(osg::NodeVisitor* nv, osg::Drawable* drawable);

    private:
        std::vector<Nif::NiMorphData::MorphData> mMorphs;
    };

    class KeyframeController : public osg::NodeCallback, public Controller, public ValueInterpolator
    {
    private:
        const Nif::QuaternionKeyMap* mRotations;
        const Nif::FloatKeyMap* mXRotations;
        const Nif::FloatKeyMap* mYRotations;
        const Nif::FloatKeyMap* mZRotations;
        const Nif::Vector3KeyMap* mTranslations;
        const Nif::FloatKeyMap* mScales;
        Nif::NIFFilePtr mNif; // Hold a SharedPtr to make sure key lists stay valid

        osg::Quat mInitialQuat;
        float mInitialScale;

        using ValueInterpolator::interpKey;


        osg::Quat interpKey(const Nif::QuaternionKeyMap::MapType &keys, float time);

        osg::Quat getXYZRotation(float time) const;

    public:
        /// @note The NiKeyFrameData must be valid as long as this KeyframeController exists.
        KeyframeController(const Nif::NIFFilePtr& nif, const Nif::NiKeyframeData *data,
              osg::Quat initialQuat, float initialScale);
        KeyframeController();
        KeyframeController(const KeyframeController& copy, const osg::CopyOp& copyop);

        META_Object(NifOsg, KeyframeController)

        virtual osg::Vec3f getTranslation(float time) const;

        virtual void operator() (osg::Node*, osg::NodeVisitor*);
    };

    // Note we're using NodeCallback instead of StateSet::Callback because the StateSet callback doesn't support nesting
    struct UVController : public osg::NodeCallback, public Controller, public ValueInterpolator
    {
    public:
        UVController();
        UVController(const UVController&,const osg::CopyOp& = osg::CopyOp::SHALLOW_COPY);
        UVController(const Nif::NiUVData *data, std::set<int> textureUnits);

        META_Object(NifOsg,UVController)

        virtual void operator() (osg::Node*, osg::NodeVisitor*);

    private:
        Nif::FloatKeyMap mUTrans;
        Nif::FloatKeyMap mVTrans;
        Nif::FloatKeyMap mUScale;
        Nif::FloatKeyMap mVScale;
        std::set<int> mTextureUnits;
    };

    class VisController : public osg::NodeCallback, public Controller
    {
    private:
        std::vector<Nif::NiVisData::VisData> mData;

        bool calculate(float time) const;

    public:
        VisController(const Nif::NiVisData *data);
        VisController();
        VisController(const VisController& copy, const osg::CopyOp& copyop);

        META_Object(NifOsg, VisController)

        virtual void operator() (osg::Node* node, osg::NodeVisitor* nv);
    };

    class AlphaController : public osg::NodeCallback, public Controller, public ValueInterpolator
    {
    private:
        Nif::FloatKeyMap mData;

    public:
        AlphaController(const Nif::NiFloatData *data);
        AlphaController();
        AlphaController(const AlphaController& copy, const osg::CopyOp& copyop);

        virtual void operator() (osg::Node* node, osg::NodeVisitor* nv);

        META_Object(NifOsg, AlphaController)
    };

    class MaterialColorController : public osg::NodeCallback, public Controller, public ValueInterpolator
    {
    private:
        Nif::Vector3KeyMap mData;

    public:
        MaterialColorController(const Nif::NiPosData *data);
        MaterialColorController();
        MaterialColorController(const MaterialColorController& copy, const osg::CopyOp& copyop);

        META_Object(NifOsg, MaterialColorController)

        virtual void operator() (osg::Node* node, osg::NodeVisitor* nv);
    };

    // untested
    class FlipController : public osg::NodeCallback, public Controller
    {
    private:
        int mTexSlot;
        float mDelta;
        std::vector<osg::ref_ptr<osg::Image> > mTextures;

    public:
        FlipController(const Nif::NiFlipController* ctrl, std::vector<osg::ref_ptr<osg::Image> > textures);
        FlipController();
        FlipController(const FlipController& copy, const osg::CopyOp& copyop);

        META_Object(NifOsg, FlipController)

        virtual void operator() (osg::Node* node, osg::NodeVisitor* nv);
    };

    class ParticleSystemController : public osg::NodeCallback, public Controller
    {
    public:
        ParticleSystemController(const Nif::NiParticleSystemController* ctrl);
        ParticleSystemController();
        ParticleSystemController(const ParticleSystemController& copy, const osg::CopyOp& copyop);

        META_Object(NifOsg, ParticleSystemController)

        virtual void operator() (osg::Node* node, osg::NodeVisitor* nv);

    private:
        float mEmitStart;
        float mEmitStop;
    };

}

#endif