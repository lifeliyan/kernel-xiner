#define LIGHT_DEV_NAME    "light_tc"
#define LIGHT_DEVICE_MISC_NAME  "light_misc_tc"
#define POLL_INTERVAL_DEF  (200)
#define MS_TO_NS(x)			(x*1000000L)


struct light_tc_status {
	atomic_t enabled_light;
	struct hrtimer hr_timer_light;
        ktime_t ktime_light;

	struct work_struct work_light;
	struct workqueue_struct *light_wq;

	struct iio_channel *chan;

};
