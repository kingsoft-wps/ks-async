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


extern void __forcelink_to_ks_raw_future_cpp();
extern void __forcelink_to_ks_raw_promise_cpp();
extern void __forcelink_to_ks_future_util_cpp();
extern void __forcelink_to_ks_raw_flow_cpp();
extern void __forcelink_to_ks_apartment_cpp();
extern void __forcelink_to_ks_cancel_inspector_cpp();
extern void __forcelink_to_ks_single_thread_apartment_imp_cpp();
extern void __forcelink_to_ks_thread_pool_apartment_imp_cpp();
extern void __forcelink_to_ks_notification_center_cpp();

void __ks_async_forcelinks() {
    __forcelink_to_ks_raw_future_cpp();
    __forcelink_to_ks_raw_promise_cpp();
    __forcelink_to_ks_future_util_cpp();
    __forcelink_to_ks_raw_flow_cpp();
    __forcelink_to_ks_apartment_cpp();
    __forcelink_to_ks_cancel_inspector_cpp();
    __forcelink_to_ks_single_thread_apartment_imp_cpp();
    __forcelink_to_ks_thread_pool_apartment_imp_cpp();
    __forcelink_to_ks_notification_center_cpp();
}
