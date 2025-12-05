/* Copyright 2024 The Kingsoft's ks-async Authors. All Rights Reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
==============================================================================*/

#include "test_base.h"

TEST(test_notification_center_suite, test_notification_center) {
    ks_waitgroup work_wg(0);

    std::atomic<int> ab_match{ 0 };
    std::atomic<int> a_match{ 0 };
    uint64_t observer_id = 0;

     struct {} sender, observer;
    ks_notification_center::default_center()->add_observer(&observer, "a.b.c.d", ks_apartment::default_mta(), [](const ks_notification& notification) {
        ASSERT(false);
        });

    observer_id=ks_notification_center::default_center()->add_observer(&observer, "a.b.*", ks_apartment::default_mta(), [&ab_match,&work_wg](const ks_notification& notification) {
        ab_match++;
        work_wg.done();
        });

    ks_notification_center::default_center()->add_observer(&observer, "a.*", ks_apartment::default_mta(), [&a_match,&work_wg](const ks_notification& notification) {
        a_match++;
        work_wg.done();
        });

    ks_notification_center::default_center()->post_notification(&sender, "a.x.y.z");
    work_wg.add(1);
    
    ks_notification_center::default_center()->post_notification_with_payload<int>(&sender, "a.x.y.z", 1);
    work_wg.add(1);
    
    ks_notification_center::default_center()->post_notification_with_payload<std::string>(&sender, "a.b.c.e", "xxx");
    work_wg.add(2);
    
    ks_notification_center::default_center()->post_notification_with_payload<std::string>(&sender, "a.b.c", "xxx");
    work_wg.add(2);
    
    ks_notification_center::default_center()->post_notification_with_payload<std::string>(&sender, "a.c", "xxx");
    work_wg.add(1);

    work_wg.wait();

    ks_notification_center::default_center()->remove_observer(&observer, observer_id);

    ks_notification_center::default_center()->post_notification_with_payload<std::string>(&sender, "a.b.d", "xxx");
    work_wg.add(1);

    work_wg.wait();
    ks_notification_center::default_center()->remove_observer(&observer);
    ks_notification_center::default_center()->remove_observer(&sender);

    EXPECT_EQ(ab_match, 2);
    EXPECT_EQ(a_match, 6);
}
