#include "sin_compat/difficulty_state_archive_contract.hpp"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <vector>
using namespace sin_compat::difficulty;

int main() {
    static_assert(archive_metric_count==14 && archive_modifier_pair_count==10);
    static_assert(archive_modifier_scalar_count==20 && archive_advisor_object_count==6);
    ManagerStateArchiveFields reference{};
    for(std::size_t i=0;i<14;++i)reference.metrics[i]=static_cast<float>(i)+0.25f;
    for(std::size_t i=0;i<10;++i)
        reference.modifiers[i]={{static_cast<float>(i),-static_cast<float>(i)-0.5f}};
    const auto metrics=snapshot_metric_scalars(reference);
    const auto modifiers=snapshot_modifier_scalars(reference);
    assert(metrics.size()==14 && modifiers.size()==20);
    for(std::size_t i=0;i<10;++i) {
        assert(modifiers[2*i]==reference.modifiers[i][0]);
        assert(modifiers[2*i+1]==reference.modifiers[i][1]);
    }
    ManagerStateArchiveFields restored{};
    assert(apply_metric_scalars(restored,14,metrics));
    assert(apply_modifier_scalars(restored,20,modifiers));
    assert(restored.metrics==reference.metrics);
    assert(restored.modifiers==reference.modifiers);
    auto extended_metrics=metrics;extended_metrics.push_back(987.0f);
    auto extended_modifiers=modifiers;
    extended_modifiers.push_back(998.0f);extended_modifiers.push_back(999.0f);
    assert(apply_metric_scalars(restored,15,extended_metrics));
    assert(apply_modifier_scalars(restored,22,extended_modifiers));
    assert(restored.metrics==reference.metrics && restored.modifiers==reference.modifiers);
    ManagerStateArchiveFields odd{};
    odd.modifiers[9]={{777.0f,888.0f}};
    assert(apply_modifier_scalars(odd,19,modifiers));
    for(std::size_t i=0;i<9;++i)assert(odd.modifiers[i]==reference.modifiers[i]);
    assert(odd.modifiers[9][0]==777.0f && odd.modifiers[9][1]==888.0f);
    ManagerStateArchiveFields partial{};
    partial.metrics.fill(42.0f);
    assert(apply_metric_scalars(partial,3,metrics));
    for(std::size_t i=0;i<14;++i)
        assert(partial.metrics[i]==(i<3?metrics[i]:42.0f));
    auto before=restored;
    assert(!apply_metric_scalars(restored,14,std::vector<float>(2,1.0f)));
    assert(!apply_modifier_scalars(restored,20,std::vector<float>(19,1.0f)));
    assert(!apply_metric_scalars(restored,-1,metrics));
    assert(!apply_modifier_scalars(restored,-1,modifiers));
    assert(restored.metrics==before.metrics && restored.modifiers==before.modifiers);
    assert(advisor_objects_to_restore(-100)==0);
    assert(advisor_objects_to_restore(0)==0);
    assert(advisor_objects_to_restore(3)==3);
    assert(advisor_objects_to_restore(6)==6);
    assert(advisor_objects_to_restore(100)==6);
}
