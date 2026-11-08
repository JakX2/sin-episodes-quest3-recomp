#include "sin_compat/original_parent_attachment.hpp"
#include <cassert>
#include <cmath>
#include <iostream>
#include <map>
using namespace sin_compat::jessica;
void near(float x,float y){assert(std::fabs(x-y)<0.003f);}
struct World final:IOriginalWorldTransformHost {
    std::map<WorldEntityId,WorldTransformSnapshot> nodes;
    std::optional<WorldTransformSnapshot> snapshot(WorldEntityId id) override {
        auto it=nodes.find(id);
        return it==nodes.end()?std::nullopt:
            std::optional<WorldTransformSnapshot>(it->second);
    }
    std::optional<WorldMatrix3x4> attachment_parent_matrix(
        WorldEntityId,WorldEntityId,std::uint8_t) override {
        assert(false&&"adapter must own the attachment callback");
        return std::nullopt;
    }
    bool commit_world_transform(WorldEntityId id,
                                const WorldTransformSnapshot& value) override {
        nodes.at(id)=value;return true;
    }
    bool on_world_transform_rebuilt(WorldEntityId) override {return true;}
};
struct Attach final:IOriginalNativeAttachmentSource {
    bool animating=true;
    int lookups=0;
    std::optional<OriginalAttachmentPose> pose;
    bool has_animating_interface(WorldEntityId id) override {
        assert(id==7);return animating;
    }
    std::optional<OriginalAttachmentPose> get_attachment_pose(
        WorldEntityId id,std::uint8_t slot) override {
        assert(id==7&&slot==5);++lookups;return pose;
    }
};
int main() {
    World source;Attach attachments;
    WorldTransformSnapshot p{};
    p.local_origin={10,20,30};p.local_angles={0,90,0};p.dirty=true;
    source.nodes[7]=p;
    WorldTransformSnapshot c{};
    c.parent=7;c.parent_attachment=5;c.local_origin={2,0,0};
    c.dirty=true;source.nodes[8]=c;
    OriginalAttachmentWorldTransformAdapter decorated(source,attachments);
    OriginalParentMatrixWorldAdapter adapter(decorated,attachments);
    auto m=resolve_original_parent_matrix(adapter,8);
    assert(m.route==OriginalParentMatrixRoute::ParentMatrix);
    near(m.matrix.row_major[3],10);near(m.matrix.row_major[7],20);
    auto r=refresh_original_world_transform(decorated,8);
    assert(r.successful());
    near(r.world_origin.x,10);near(r.world_origin.y,22);
    near(r.world_origin.z,30);
    assert(attachments.lookups==2);
    source.nodes[8].dirty=true;
    attachments.pose=OriginalAttachmentPose{{100,200,300},{0,90,0}};
    m=resolve_original_parent_matrix(adapter,8);
    assert(m.route==OriginalParentMatrixRoute::AttachmentMatrix);
    near(m.matrix.row_major[3],100);
    r=refresh_original_world_transform(decorated,8);
    assert(r.successful());
    near(r.world_origin.x,100);near(r.world_origin.y,202);
    near(r.world_origin.z,300);
    source.nodes.erase(7);source.nodes[8].dirty=true;
    m=resolve_original_parent_matrix(adapter,8);
    assert(m.route==OriginalParentMatrixRoute::Identity);
    r=refresh_original_world_transform(decorated,8);
    assert(r.successful());near(r.world_origin.x,2);near(r.world_origin.y,0);
    source.nodes[7]=p;source.nodes[8].dirty=true;attachments.animating=false;
    m=resolve_original_parent_matrix(adapter,8);
    assert(m.route==OriginalParentMatrixRoute::ParentMatrix);
    std::cout<<"Original attachment selector and world transform bridge PASS\n";
}
