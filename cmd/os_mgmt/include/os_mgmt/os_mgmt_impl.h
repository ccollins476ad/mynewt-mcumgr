#ifndef H_OS_MGMT_IMPL_
#define H_OS_MGMT_IMPL_

struct os_mgmt_task_info;

/**
 * @brief Retrieves information about the specified task.  
 *
 * @param idx                   The index of the task to query.
 * @param out_info              On success, the requested information gets
 *                                  written here.
 *
 * @return                      0 on success;
 *                              MGMT_ERR_ENOENT if no such task exists;
 *                              Other MGMT_ERR_[...] code on failure.
 */
int os_mgmt_impl_task_info(int idx, struct os_mgmt_task_info *out_info);

/**
 * @brief Schedules a near-immediate system reset.  There must be a slight
 * delay before the reset occurs to allow time for the mgmt response to be
 * delivered.
 *
 * @return                      0 on success, MGMT_ERR_[...] code on failure.
 */
int os_mgmt_impl_reset(void);

#endif
