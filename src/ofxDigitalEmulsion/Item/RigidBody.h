#pragma once

#include "../Graph/Node.h"
#include "../../../addons/ofxCvMin/src/ofxCvMin.h"

namespace ofxDigitalEmulsion {
	namespace Item {
		class RigidBody : public Graph::Node {
		public:
			RigidBody();
			virtual string getTypeName() const override;
			void init();
			void drawWorld() override;
			virtual void drawObject() { }

			ofMatrix4x4 getTransform() const;
			void setTransform(const ofMatrix4x4 &);
			void setExtrinsics(cv::Mat rotation, cv::Mat translation);

		protected:
			ofParameter<float> translation[3];
			ofParameter<float> rotationEuler[3];

		private:
			void serialize(Json::Value &);
			void deserialize(const Json::Value &);
			void populateInspector(ofxCvGui::ElementGroupPtr);
		};

		ofVec3f toEuler(const ofQuaternion &);
		ofQuaternion toQuaternion(const ofVec3f &);
	}
}