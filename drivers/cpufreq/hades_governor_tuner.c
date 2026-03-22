#include <linux/module.h>
#include <linux/sysfs.h>
#include <linux/kobject.h>
#include <linux/cpufreq.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/slab.h>

static unsigned int little_max_freq = 1690000;
static unsigned int little_min_freq = 455000;
static unsigned int big_max_freq = 2314000;
static unsigned int big_min_freq = 741000;
static char cpu_governor[32] = "schedutil";
static char io_scheduler[32] = "cfq";
static unsigned int gpu_max_freq = 900;
static unsigned int gpu_min_freq = 266;

static struct kobject *hades_perf_kobj;

static void write_sysfs_string(const char *path, const char *val)
{
    struct file *f;
    loff_t pos = 0;
    mm_segment_t old_fs;

    f = filp_open(path, O_WRONLY, 0);
    if (!IS_ERR(f)) {
        old_fs = get_fs();
        set_fs(get_ds());
        vfs_write(f, (const char __force __user *)val, strlen(val), &pos);
        set_fs(old_fs);
        filp_close(f, NULL);
    }
}

static void apply_cpu_governor(const char *gov)
{
    char path[128];
    int i;
    for (i = 0; i < 8; i++) {
        snprintf(path, sizeof(path), "/sys/devices/system/cpu/cpu%d/cpufreq/scaling_governor", i);
        write_sysfs_string(path, gov);
    }
}

static void apply_io_scheduler(const char *sched)
{
    write_sysfs_string("/sys/block/sda/queue/scheduler", sched);
    write_sysfs_string("/sys/block/mmcblk0/queue/scheduler", sched);
}

static void update_all_cpus(void)
{
    int i;
    for (i = 0; i < 8; i++) {
        cpufreq_update_policy(i);
    }
}

static ssize_t little_max_freq_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf) { return sprintf(buf, "%u\n", little_max_freq); }
static ssize_t little_max_freq_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count) {
    sscanf(buf, "%u", &little_max_freq);
    update_all_cpus();
    return count;
}
static struct kobj_attribute little_max_freq_attr = __ATTR(little_max_freq, 0644, little_max_freq_show, little_max_freq_store);

static ssize_t little_min_freq_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf) { return sprintf(buf, "%u\n", little_min_freq); }
static ssize_t little_min_freq_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count) {
    sscanf(buf, "%u", &little_min_freq);
    update_all_cpus();
    return count;
}
static struct kobj_attribute little_min_freq_attr = __ATTR(little_min_freq, 0644, little_min_freq_show, little_min_freq_store);

static ssize_t big_max_freq_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf) { return sprintf(buf, "%u\n", big_max_freq); }
static ssize_t big_max_freq_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count) {
    sscanf(buf, "%u", &big_max_freq);
    update_all_cpus();
    return count;
}
static struct kobj_attribute big_max_freq_attr = __ATTR(big_max_freq, 0644, big_max_freq_show, big_max_freq_store);

static ssize_t big_min_freq_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf) { return sprintf(buf, "%u\n", big_min_freq); }
static ssize_t big_min_freq_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count) {
    sscanf(buf, "%u", &big_min_freq);
    update_all_cpus();
    return count;
}
static struct kobj_attribute big_min_freq_attr = __ATTR(big_min_freq, 0644, big_min_freq_show, big_min_freq_store);

static ssize_t cpu_governor_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf) { return sprintf(buf, "%s\n", cpu_governor); }
static ssize_t cpu_governor_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count) {
    sscanf(buf, "%31s", cpu_governor);
    apply_cpu_governor(cpu_governor);
    return count;
}
static struct kobj_attribute cpu_governor_attr = __ATTR(cpu_governor, 0644, cpu_governor_show, cpu_governor_store);

static ssize_t io_scheduler_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf) { return sprintf(buf, "%s\n", io_scheduler); }
static ssize_t io_scheduler_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count) {
    sscanf(buf, "%31s", io_scheduler);
    apply_io_scheduler(io_scheduler);
    return count;
}
static struct kobj_attribute io_scheduler_attr = __ATTR(io_scheduler, 0644, io_scheduler_show, io_scheduler_store);

static ssize_t gpu_max_freq_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf) { return sprintf(buf, "%u\n", gpu_max_freq); }
static ssize_t gpu_max_freq_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count) {
    sscanf(buf, "%u", &gpu_max_freq);
    return count;
}
static struct kobj_attribute gpu_max_freq_attr = __ATTR(gpu_max_freq, 0644, gpu_max_freq_show, gpu_max_freq_store);

static ssize_t gpu_min_freq_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf) { return sprintf(buf, "%u\n", gpu_min_freq); }
static ssize_t gpu_min_freq_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count) {
    sscanf(buf, "%u", &gpu_min_freq);
    return count;
}
static struct kobj_attribute gpu_min_freq_attr = __ATTR(gpu_min_freq, 0644, gpu_min_freq_show, gpu_min_freq_store);

static struct attribute *hades_perf_attrs[] = {
    &little_max_freq_attr.attr,
    &little_min_freq_attr.attr,
    &big_max_freq_attr.attr,
    &big_min_freq_attr.attr,
    &cpu_governor_attr.attr,
    &io_scheduler_attr.attr,
    &gpu_max_freq_attr.attr,
    &gpu_min_freq_attr.attr,
    NULL,
};

static struct attribute_group hades_perf_attr_group = {
    .attrs = hades_perf_attrs,
};

static int hades_cpufreq_notifier(struct notifier_block *nb, unsigned long val, void *data)
{
    struct cpufreq_policy *policy = data;
    if (val == CPUFREQ_ADJUST) {
        if (policy->cpu < 4) {
            if (policy->max > little_max_freq) policy->max = little_max_freq;
            if (policy->min < little_min_freq) policy->min = little_min_freq;
        } else {
            if (policy->max > big_max_freq) policy->max = big_max_freq;
            if (policy->min < big_min_freq) policy->min = big_min_freq;
        }
    }
    return 0;
}

static struct notifier_block hades_cpufreq_nb = {
    .notifier_call = hades_cpufreq_notifier,
};

static int __init hades_governor_tuner_init(void)
{
    int ret;

    hades_perf_kobj = kobject_create_and_add("hades_perf", kernel_kobj);
    if (!hades_perf_kobj)
        return -ENOMEM;

    ret = sysfs_create_group(hades_perf_kobj, &hades_perf_attr_group);
    if (ret) {
        kobject_put(hades_perf_kobj);
        return ret;
    }

    cpufreq_register_notifier(&hades_cpufreq_nb, CPUFREQ_POLICY_NOTIFIER);

    return 0;
}

static void __exit hades_governor_tuner_exit(void)
{
    cpufreq_unregister_notifier(&hades_cpufreq_nb, CPUFREQ_POLICY_NOTIFIER);
    sysfs_remove_group(hades_perf_kobj, &hades_perf_attr_group);
    kobject_put(hades_perf_kobj);
}

module_init(hades_governor_tuner_init);
module_exit(hades_governor_tuner_exit);

MODULE_LICENSE("GPL");
