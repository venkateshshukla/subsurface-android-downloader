/* profile.c */
/* creates all the necessary data for drawing the dive profile
 */
#include "gettext.h"
#include <limits.h>
#include <string.h>

#include "dive.h"
#include "display.h"
#include "divelist.h"

#include "profile.h"
#include "deco.h"
#include "libdivecomputer/parser.h"
#include "libdivecomputer/version.h"
#include "membuffer.h"

int selected_dive = -1; /* careful: 0 is a valid value */
unsigned int dc_number = 0;


static struct plot_data *last_pi_entry_new = NULL;

#ifdef DEBUG_PI
/* debugging tool - not normally used */
static void dump_pi(struct plot_info *pi)
{
	int i;

	printf("pi:{nr:%d maxtime:%d meandepth:%d maxdepth:%d \n"
	       "    maxpressure:%d mintemp:%d maxtemp:%d\n",
	       pi->nr, pi->maxtime, pi->meandepth, pi->maxdepth,
	       pi->maxpressure, pi->mintemp, pi->maxtemp);
	for (i = 0; i < pi->nr; i++) {
		struct plot_data *entry = &pi->entry[i];
		printf("    entry[%d]:{cylinderindex:%d sec:%d pressure:{%d,%d}\n"
		       "                time:%d:%02d temperature:%d depth:%d stopdepth:%d stoptime:%d ndl:%d smoothed:%d po2:%lf phe:%lf pn2:%lf sum-pp %lf}\n",
		       i, entry->cylinderindex, entry->sec,
		       entry->pressure[0], entry->pressure[1],
		       entry->sec / 60, entry->sec % 60,
		       entry->temperature, entry->depth, entry->stopdepth, entry->stoptime, entry->ndl, entry->smoothed,
		       entry->po2, entry->phe, entry->pn2,
		       entry->po2 + entry->phe + entry->pn2);
	}
	printf("   }\n");
}
#endif

#define ROUND_UP(x, y) ((((x) + (y) - 1) / (y)) * (y))
#define DIV_UP(x, y) (((x) + (y) - 1) / (y))

/*
 * When showing dive profiles, we scale things to the
 * current dive. However, we don't scale past less than
 * 30 minutes or 90 ft, just so that small dives show
 * up as such unless zoom is enabled.
 * We also need to add 180 seconds at the end so the min/max
 * plots correctly
 */
int get_maxtime(struct plot_info *pi)
{
	int seconds = pi->maxtime;
	if (prefs.zoomed_plot) {
		/* Rounded up to one minute, with at least 2.5 minutes to
		 * spare.
		 * For dive times shorter than 10 minutes, we use seconds/4 to
		 * calculate the space dynamically.
		 * This is seamless since 600/4 = 150.
		 */
		if (seconds < 600)
			return ROUND_UP(seconds + seconds / 4, 60);
		else
			return ROUND_UP(seconds + 150, 60);
	} else {
		/* min 30 minutes, rounded up to 5 minutes, with at least 2.5 minutes to spare */
		return MAX(30 * 60, ROUND_UP(seconds + 150, 60 * 5));
	}
}

/* get the maximum depth to which we want to plot
 * take into account the additional vertical space needed to plot
 * partial pressure graphs */
int get_maxdepth(struct plot_info *pi)
{
	unsigned mm = pi->maxdepth;
	int md;

	if (prefs.zoomed_plot) {
		/* Rounded up to 10m, with at least 3m to spare */
		md = ROUND_UP(mm + 3000, 10000);
	} else {
		/* Minimum 30m, rounded up to 10m, with at least 3m to spare */
		md = MAX((unsigned)30000, ROUND_UP(mm + 3000, 10000));
	}
	md += pi->maxpp * 9000;
	return md;
}

/* collect all event names and whether we display them */
struct ev_select *ev_namelist;
int evn_allocated;
int evn_used;

#if WE_DONT_USE_THIS /* we need to implement event filters in Qt */
int evn_foreach(void (*callback)(const char *, bool *, void *), void *data) {
	int i;

	for (i = 0; i < evn_used; i++) {
		/* here we display an event name on screen - so translate */
		callback(translate("gettextFromC", ev_namelist[i].ev_name), &ev_namelist[i].plot_ev, data);
	}
	return i;
}
#endif /* WE_DONT_USE_THIS */

void clear_events(void)
{
	evn_used = 0;
}

void remember_event(const char *eventname)
{
	int i = 0, len;

	if (!eventname || (len = strlen(eventname)) == 0)
		return;
	while (i < evn_used) {
		if (!strncmp(eventname, ev_namelist[i].ev_name, len))
			return;
		i++;
	}
	if (evn_used == evn_allocated) {
		evn_allocated += 10;
		ev_namelist = realloc(ev_namelist, evn_allocated * sizeof(struct ev_select));
		if (!ev_namelist)
			/* we are screwed, but let's just bail out */
			return;
	}
	ev_namelist[evn_used].ev_name = strdup(eventname);
	ev_namelist[evn_used].plot_ev = true;
	evn_used++;
}

/* Get local sac-rate (in ml/min) between entry1 and entry2 */
static int get_local_sac(struct plot_data *entry1, struct plot_data *entry2, struct dive *dive)
{
	int index = entry1->cylinderindex;
	cylinder_t *cyl;
	int duration = entry2->sec - entry1->sec;
	int depth, airuse;
	pressure_t a, b;
	double atm;

	if (entry2->cylinderindex != index)
		return 0;
	if (duration <= 0)
		return 0;
	a.mbar = GET_PRESSURE(entry1);
	b.mbar = GET_PRESSURE(entry2);
	if (!b.mbar || a.mbar <= b.mbar)
		return 0;

	/* Mean pressure in ATM */
	depth = (entry1->depth + entry2->depth) / 2;
	atm = depth_to_atm(depth, dive);

	cyl = dive->cylinder + index;

	airuse = gas_volume(cyl, a) - gas_volume(cyl, b);

	/* milliliters per minute */
	return airuse / atm * 60 / duration;
}

static void analyze_plot_info_minmax_minute(struct plot_data *entry, struct plot_data *first, struct plot_data *last, int index)
{
	struct plot_data *p = entry;
	int time = entry->sec;
	int seconds = 90 * (index + 1);
	struct plot_data *min, *max;
	int avg, nr;

	/* Go back 'seconds' in time */
	while (p > first) {
		if (p[-1].sec < time - seconds)
			break;
		p--;
	}

	/* Then go forward until we hit an entry past the time */
	min = max = p;
	avg = p->depth;
	nr = 1;
	while (++p < last) {
		int depth = p->depth;
		if (p->sec > time + seconds)
			break;
		avg += depth;
		nr++;
		if (depth < min->depth)
			min = p;
		if (depth > max->depth)
			max = p;
	}
	entry->min[index] = min;
	entry->max[index] = max;
	entry->avg[index] = (avg + nr / 2) / nr;
}

static void analyze_plot_info_minmax(struct plot_data *entry, struct plot_data *first, struct plot_data *last)
{
	analyze_plot_info_minmax_minute(entry, first, last, 0);
	analyze_plot_info_minmax_minute(entry, first, last, 1);
	analyze_plot_info_minmax_minute(entry, first, last, 2);
}

static velocity_t velocity(int speed)
{
	velocity_t v;

	if (speed < -304) /* ascent faster than -60ft/min */
		v = CRAZY;
	else if (speed < -152) /* above -30ft/min */
		v = FAST;
	else if (speed < -76) /* -15ft/min */
		v = MODERATE;
	else if (speed < -25) /* -5ft/min */
		v = SLOW;
	else if (speed < 25) /* very hard to find data, but it appears that the recommendations
				for descent are usually about 2x ascent rate; still, we want
				stable to mean stable */
		v = STABLE;
	else if (speed < 152) /* between 5 and 30ft/min is considered slow */
		v = SLOW;
	else if (speed < 304) /* up to 60ft/min is moderate */
		v = MODERATE;
	else if (speed < 507) /* up to 100ft/min is fast */
		v = FAST;
	else /* more than that is just crazy - you'll blow your ears out */
		v = CRAZY;

	return v;
}

struct plot_info *analyze_plot_info(struct plot_info *pi)
{
	int i;
	int nr = pi->nr;

	/* Smoothing function: 5-point triangular smooth */
	for (i = 2; i < nr; i++) {
		struct plot_data *entry = pi->entry + i;
		int depth;

		if (i < nr - 2) {
			depth = entry[-2].depth + 2 * entry[-1].depth + 3 * entry[0].depth + 2 * entry[1].depth + entry[2].depth;
			entry->smoothed = (depth + 4) / 9;
		}
		/* vertical velocity in mm/sec */
		/* Linus wants to smooth this - let's at least look at the samples that aren't FAST or CRAZY */
		if (entry[0].sec - entry[-1].sec) {
			entry->speed = (entry[0].depth - entry[-1].depth) / (entry[0].sec - entry[-1].sec);
			entry->velocity = velocity(entry->speed);
			/* if our samples are short and we aren't too FAST*/
			if (entry[0].sec - entry[-1].sec < 15 && entry->velocity < FAST) {
				int past = -2;
				while (i + past > 0 && entry[0].sec - entry[past].sec < 15)
					past--;
				entry->velocity = velocity((entry[0].depth - entry[past].depth) /
							   (entry[0].sec - entry[past].sec));
			}
		} else {
			entry->velocity = STABLE;
			entry->speed = 0;
		}
	}

	/* One-, two- and three-minute minmax data */
	for (i = 0; i < nr; i++) {
		struct plot_data *entry = pi->entry + i;
		analyze_plot_info_minmax(entry, pi->entry, pi->entry + nr);
	}

	return pi;
}

/*
 * simple structure to track the beginning and end tank pressure as
 * well as the integral of depth over time spent while we have no
 * pressure reading from the tank */
typedef struct pr_track_struct pr_track_t;
struct pr_track_struct {
	int start;
	int end;
	int t_start;
	int t_end;
	int pressure_time;
	pr_track_t *next;
};

static pr_track_t *pr_track_alloc(int start, int t_start)
{
	pr_track_t *pt = malloc(sizeof(pr_track_t));
	pt->start = start;
	pt->end = 0;
	pt->t_start = pt->t_end = t_start;
	pt->pressure_time = 0;
	pt->next = NULL;
	return pt;
}

/* poor man's linked list */
static pr_track_t *list_last(pr_track_t *list)
{
	pr_track_t *tail = list;
	if (!tail)
		return NULL;
	while (tail->next) {
		tail = tail->next;
	}
	return tail;
}

static pr_track_t *list_add(pr_track_t *list, pr_track_t *element)
{
	pr_track_t *tail = list_last(list);
	if (!tail)
		return element;
	tail->next = element;
	return list;
}

static void list_free(pr_track_t *list)
{
	if (!list)
		return;
	list_free(list->next);
	free(list);
}

#ifdef DEBUG_PR_TRACK
static void dump_pr_track(pr_track_t **track_pr)
{
	int cyl;
	pr_track_t *list;

	for (cyl = 0; cyl < MAX_CYLINDERS; cyl++) {
		list = track_pr[cyl];
		while (list) {
			printf("cyl%d: start %d end %d t_start %d t_end %d pt %d\n", cyl,
			       list->start, list->end, list->t_start, list->t_end, list->pressure_time);
			list = list->next;
		}
	}
}
#endif

typedef struct pr_interpolate_struct pr_interpolate_t;
struct pr_interpolate_struct {
	int start;
	int end;
	int pressure_time;
	int acc_pressure_time;
};

#ifdef DEBUG_PR_INTERPOLATE
static void dump_pr_interpolate(int i, pr_interpolate_t interpolate_pr)
{
	printf("Interpolate for entry %d: start %d - end %d - pt %d - acc_pt %d\n", i,
	       interpolate_pr.start, interpolate_pr.end, interpolate_pr.pressure_time, interpolate_pr.acc_pressure_time);
}
#endif

/*
 * This looks at the pressures for one cylinder, and
 * calculates any missing beginning/end pressures for
 * each segment by taking the over-all SAC-rate into
 * account for that cylinder.
 *
 * NOTE! Many segments have full pressure information
 * (both beginning and ending pressure). But if we have
 * switched away from a cylinder, we will have the
 * beginning pressure for the first segment with a
 * missing end pressure. We may then have one or more
 * segments without beginning or end pressures, until
 * we finally have a segment with an end pressure.
 *
 * We want to spread out the pressure over these missing
 * segments according to how big of a time_pressure area
 * they have.
 */
static void fill_missing_segment_pressures(pr_track_t *list)
{
	while (list) {
		int start = list->start, end;
		pr_track_t *tmp = list;
		int pt_sum = 0, pt = 0;

		for (;;) {
			pt_sum += tmp->pressure_time;
			end = tmp->end;
			if (end)
				break;
			end = start;
			if (!tmp->next)
				break;
			tmp = tmp->next;
		}

		if (!start)
			start = end;

		/*
		 * Now 'start' and 'end' contain the pressure values
		 * for the set of segments described by 'list'..'tmp'.
		 * pt_sum is the sum of all the pressure-times of the
		 * segments.
		 *
		 * Now dole out the pressures relative to pressure-time.
		 */
		list->start = start;
		tmp->end = end;
		for (;;) {
			int pressure;
			pt += list->pressure_time;
			pressure = start;
			if (pt_sum)
				pressure -= (start - end) * (double)pt / pt_sum;
			list->end = pressure;
			if (list == tmp)
				break;
			list = list->next;
			list->start = pressure;
		}

		/* Ok, we've done that set of segments */
		list = list->next;
	}
}

/*
 * What's the pressure-time between two plot data entries?
 * We're calculating the integral of pressure over time by
 * adding these up.
 *
 * The units won't matter as long as everybody agrees about
 * them, since they'll cancel out - we use this to calculate
 * a constant SAC-rate-equivalent, but we only use it to
 * scale pressures, so it ends up being a unitless scaling
 * factor.
 */
static inline int pressure_time(struct dive *dive, struct divecomputer *dc, struct plot_data *a, struct plot_data *b)
{
	int time = b->sec - a->sec;
	int depth = (a->depth + b->depth) / 2;

	if (depth <= SURFACE_THRESHOLD)
		return 0;

	return depth_to_mbar(depth, dive) * time;
}

static struct pr_interpolate_struct get_pr_interpolate_data(pr_track_t *segment, struct plot_info *pi, int cur)
{
	struct pr_interpolate_struct interpolate;
	int i;
	struct plot_data *entry;

	interpolate.start = segment->start;
	interpolate.end = segment->end;
	interpolate.acc_pressure_time = 0;
	interpolate.pressure_time = 0;

	for (i = 0; i < pi->nr; i++) {
		entry = pi->entry + i;
		if (entry->sec < segment->t_start)
			continue;
		if (entry->sec >= segment->t_end) {
			interpolate.pressure_time += entry->pressure_time;
			break;
		}
		if (entry->sec == segment->t_start) {
			interpolate.acc_pressure_time = 0;
			interpolate.pressure_time = 0;
			if (SENSOR_PRESSURE(entry))
				interpolate.start = SENSOR_PRESSURE(entry);
			continue;
		}
		if (i < cur) {
			if (SENSOR_PRESSURE(entry)) {
				interpolate.start = SENSOR_PRESSURE(entry);
				interpolate.acc_pressure_time = 0;
				interpolate.pressure_time = 0;
			} else {
				interpolate.acc_pressure_time += entry->pressure_time;
				interpolate.pressure_time += entry->pressure_time;
			}
			continue;
		}
		if (i == cur) {
			interpolate.acc_pressure_time += entry->pressure_time;
			interpolate.pressure_time += entry->pressure_time;
			continue;
		}
		interpolate.pressure_time += entry->pressure_time;
		if (SENSOR_PRESSURE(entry)) {
			interpolate.end = SENSOR_PRESSURE(entry);
			break;
		}
	}
	return interpolate;
}

static void fill_missing_tank_pressures(struct dive *dive, struct plot_info *pi, pr_track_t **track_pr)
{
	int cyl, i;
	struct plot_data *entry;
	int cur_pr[MAX_CYLINDERS];

#ifdef DEBUG_PR_TRACK
	/* another great debugging tool */
	dump_pr_track(track_pr);
#endif
	for (cyl = 0; cyl < MAX_CYLINDERS; cyl++) {
		if (!track_pr[cyl]) {
			/* no segment where this cylinder is used */
			cur_pr[cyl] = -1;
			continue;
		}
		fill_missing_segment_pressures(track_pr[cyl]);
		cur_pr[cyl] = track_pr[cyl]->start;
	}

	/* The first two are "fillers", but in case we don't have a sample
	 * at time 0 we need to process the second of them here */
	for (i = 1; i < pi->nr; i++) {
		double magic;
		pr_track_t *segment;
		pr_interpolate_t interpolate;

		entry = pi->entry + i;
		cyl = entry->cylinderindex;

		if (SENSOR_PRESSURE(entry)) {
			cur_pr[cyl] = SENSOR_PRESSURE(entry);
			continue;
		}

		/* Find the right pressure segment for this entry.. */
		segment = track_pr[cyl];
		while (segment && segment->t_end < entry->sec)
			segment = segment->next;

		/* No (or empty) segment? Just use our current pressure */
		if (!segment || !segment->pressure_time) {
			SENSOR_PRESSURE(entry) = cur_pr[cyl];
			continue;
		}

		interpolate = get_pr_interpolate_data(segment, pi, i);
#ifdef DEBUG_PR_INTERPOLATE
		dump_pr_interpolate(i, interpolate);
#endif
		/* if this segment has pressure time, calculate a new interpolated pressure */
		if (interpolate.pressure_time) {
			/* Overall pressure change over total pressure-time for this segment*/
			magic = (interpolate.end - interpolate.start) / (double)interpolate.pressure_time;

			/* Use that overall pressure change to update the current pressure */
			cur_pr[cyl] = rint(interpolate.start + magic * interpolate.acc_pressure_time);
		}
		INTERPOLATED_PRESSURE(entry) = cur_pr[cyl];
	}
}

int get_cylinder_index(struct dive *dive, struct event *ev)
{
	int i;
	int best = 0, score = INT_MAX;
	int target_o2, target_he;
	struct gasmix *g;

	/*
	 * Crazy gas change events give us odd encoded o2/he in percent.
	 * Decode into our internal permille format.
	 */
	g = get_gasmix_from_event(ev);
	target_o2 = get_o2(g);
	target_he = get_he(g);

	/*
	 * Try to find a cylinder that best matches the target gas
	 * mix.
	 */
	for (i = 0; i < MAX_CYLINDERS; i++) {
		cylinder_t *cyl = dive->cylinder + i;
		int delta_o2, delta_he, distance;

		if (cylinder_nodata(cyl))
			continue;

		delta_o2 = get_o2(&cyl->gasmix) - target_o2;
		delta_he = get_he(&cyl->gasmix) - target_he;
		distance = delta_o2 * delta_o2;

		/* Check the event type to figure out if we should care about the he part.
		 * SAMPLE_EVENT_GASCHANGE, aka without he
		 * SAMPLE_EVENT_GASCHANGE2, aka with he
		 */
		if (ev->type == SAMPLE_EVENT_GASCHANGE2)
			distance += delta_he * delta_he;
		if (distance >= score)
			continue;
		score = distance;
		best = i;
	}
	return best;
}

struct event *get_next_event(struct event *event, char *name)
{
	if (!name || !*name)
		return NULL;
	while (event) {
		if (!strcmp(event->name, name))
			return event;
		event = event->next;
	}
	return event;
}

static int count_events(struct divecomputer *dc)
{
	int result = 0;
	struct event *ev = dc->events;
	while (ev != NULL) {
		result++;
		ev = ev->next;
	}
	return result;
}

static int set_cylinder_index(struct plot_info *pi, int i, int cylinderindex, unsigned int end)
{
	while (i < pi->nr) {
		struct plot_data *entry = pi->entry + i;
		if (entry->sec > end)
			break;
		if (entry->cylinderindex != cylinderindex) {
			entry->cylinderindex = cylinderindex;
			entry->pressure[0] = 0;
		}
		i++;
	}
	return i;
}

static void check_gas_change_events(struct dive *dive, struct divecomputer *dc, struct plot_info *pi)
{
	int i = 0, cylinderindex = 0;
	struct event *ev = get_next_event(dc->events, "gaschange");

	if (!ev)
		return;

	do {
		i = set_cylinder_index(pi, i, cylinderindex, ev->time.seconds);
		cylinderindex = get_cylinder_index(dive, ev);
		ev = get_next_event(ev->next, "gaschange");
	} while (ev);
	set_cylinder_index(pi, i, cylinderindex, ~0u);
}


struct plot_info calculate_max_limits_new(struct dive *dive, struct divecomputer *dc)
{
	static struct plot_info pi;
	int maxdepth = dive->maxdepth.mm;
	int maxtime = 0;
	int maxpressure = 0, minpressure = INT_MAX;
	int maxhr = 0, minhr = INT_MAX;
	int mintemp = dive->mintemp.mkelvin;
	int maxtemp = dive->maxtemp.mkelvin;
	int cyl;

	/* Get the per-cylinder maximum pressure if they are manual */
	for (cyl = 0; cyl < MAX_CYLINDERS; cyl++) {
		int mbar = dive->cylinder[cyl].start.mbar;
		if (mbar > maxpressure)
			maxpressure = mbar;
		if (mbar < minpressure)
			minpressure = mbar;
	}

	/* Then do all the samples from all the dive computers */
	do {
		int i = dc->samples;
		int lastdepth = 0;
		struct sample *s = dc->sample;

		while (--i >= 0) {
			int depth = s->depth.mm;
			int pressure = s->cylinderpressure.mbar;
			int temperature = s->temperature.mkelvin;
			int heartbeat = s->heartbeat;

			if (!mintemp && temperature < mintemp)
				mintemp = temperature;
			if (temperature > maxtemp)
				maxtemp = temperature;

			if (pressure && pressure < minpressure)
				minpressure = pressure;
			if (pressure > maxpressure)
				maxpressure = pressure;
			if (heartbeat > maxhr)
				maxhr = heartbeat;
			if (heartbeat < minhr)
				minhr = heartbeat;

			if (depth > maxdepth)
				maxdepth = s->depth.mm;
			if ((depth > SURFACE_THRESHOLD || lastdepth > SURFACE_THRESHOLD) &&
			    s->time.seconds > maxtime)
				maxtime = s->time.seconds;
			lastdepth = depth;
			s++;
		}
	} while ((dc = dc->next) != NULL);

	if (minpressure > maxpressure)
		minpressure = 0;
	if (minhr > maxhr)
		minhr = 0;

	memset(&pi, 0, sizeof(pi));
	pi.maxdepth = maxdepth;
	pi.maxtime = maxtime;
	pi.maxpressure = maxpressure;
	pi.minpressure = minpressure;
	pi.minhr = minhr;
	pi.maxhr = maxhr;
	pi.mintemp = mintemp;
	pi.maxtemp = maxtemp;
	return pi;
}

/* copy the previous entry (we know this exists), update time and depth
 * and zero out the sensor pressure (since this is a synthetic entry)
 * increment the entry pointer and the count of synthetic entries. */
#define INSERT_ENTRY(_time, _depth) \
	*entry = entry[-1];         \
	entry->sec = _time;         \
	entry->depth = _depth;      \
	SENSOR_PRESSURE(entry) = 0; \
	entry++;                    \
	idx++

struct plot_data *populate_plot_entries(struct dive *dive, struct divecomputer *dc, struct plot_info *pi)
{
	int idx, maxtime, nr, i;
	int lastdepth, lasttime, lasttemp = 0;
	struct plot_data *plot_data;
	struct event *ev = dc->events;

	maxtime = pi->maxtime;

	/*
	 * We want to have a plot_info event at least every 10s (so "maxtime/10+1"),
	 * but samples could be more dense than that (so add in dc->samples). We also
	 * need to have one for every event (so count events and add that) and
	 * additionally we want two surface events around the whole thing (thus the
	 * additional 4).
	 */
	nr = dc->samples + 5 + maxtime / 10 + count_events(dc);
	plot_data = calloc(nr, sizeof(struct plot_data));
	pi->entry = plot_data;
	if (!plot_data)
		return NULL;
	pi->nr = nr;
	idx = 2; /* the two extra events at the start */

	lastdepth = 0;
	lasttime = 0;
	/* skip events at time = 0 */
	while (ev && ev->time.seconds == 0)
		ev = ev->next;
	for (i = 0; i < dc->samples; i++) {
		struct plot_data *entry = plot_data + idx;
		struct sample *sample = dc->sample + i;
		int time = sample->time.seconds;
		int depth = sample->depth.mm;
		int offset, delta;

		/* Add intermediate plot entries if required */
		delta = time - lasttime;
		if (delta < 0) {
			time = lasttime;
			delta = 0;
		}
		for (offset = 10; offset < delta; offset += 10) {
			if (lasttime + offset > maxtime)
				break;

			/* Add events if they are between plot entries */
			while (ev && ev->time.seconds < lasttime + offset) {
				INSERT_ENTRY(ev->time.seconds, interpolate(lastdepth, depth, ev->time.seconds - lasttime, delta));
				ev = ev->next;
			}

			/* now insert the time interpolated entry */
			INSERT_ENTRY(lasttime + offset, interpolate(lastdepth, depth, offset, delta));

			/* skip events that happened at this time */
			while (ev && ev->time.seconds == lasttime + offset)
				ev = ev->next;
		}

		/* Add events if they are between plot entries */
		while (ev && ev->time.seconds < time) {
			INSERT_ENTRY(ev->time.seconds, interpolate(lastdepth, depth, ev->time.seconds - lasttime, delta));
			ev = ev->next;
		}

		if (time > maxtime)
			break;

		entry->sec = time;
		entry->depth = depth;

		entry->stopdepth = sample->stopdepth.mm;
		entry->stoptime = sample->stoptime.seconds;
		entry->ndl = sample->ndl.seconds;
		pi->has_ndl |= sample->ndl.seconds;
		entry->in_deco = sample->in_deco;
		entry->cns = sample->cns;
		entry->po2 = sample->po2.mbar / 1000.0;
		/* FIXME! sensor index -> cylinder index translation! */
		entry->cylinderindex = sample->sensor;
		SENSOR_PRESSURE(entry) = sample->cylinderpressure.mbar;
		if (sample->temperature.mkelvin)
			entry->temperature = lasttemp = sample->temperature.mkelvin;
		else
			entry->temperature = lasttemp;
		entry->heartbeat = sample->heartbeat;
		entry->bearing = sample->bearing.degrees;

		/* skip events that happened at this time */
		while (ev && ev->time.seconds == time)
			ev = ev->next;
		lasttime = time;
		lastdepth = depth;
		idx++;
	}

	/* Add two final surface events */
	plot_data[idx++].sec = lasttime + 1;
	plot_data[idx++].sec = lasttime + 2;
	pi->nr = idx;

	return plot_data;
}

#undef INSERT_ENTRY

static void populate_cylinder_pressure_data(int idx, int start, int end, struct plot_info *pi)
{
	int i;

	/* First: check that none of the entries has sensor pressure for this cylinder index */
	for (i = 0; i < pi->nr; i++) {
		struct plot_data *entry = pi->entry + i;
		if (entry->cylinderindex != idx)
			continue;
		if (SENSOR_PRESSURE(entry))
			return;
	}

	/* Then: populate the first entry with the beginning cylinder pressure */
	for (i = 0; i < pi->nr; i++) {
		struct plot_data *entry = pi->entry + i;
		if (entry->cylinderindex != idx)
			continue;
		SENSOR_PRESSURE(entry) = start;
		break;
	}

	/* .. and the last entry with the ending cylinder pressure */
	for (i = pi->nr; --i >= 0; /* nothing */) {
		struct plot_data *entry = pi->entry + i;
		if (entry->cylinderindex != idx)
			continue;
		SENSOR_PRESSURE(entry) = end;
		break;
	}
}

static void calculate_sac(struct dive *dive, struct plot_info *pi)
{
	int i = 0, last = 0;
	struct plot_data *last_entry = NULL;

	for (i = 0; i < pi->nr; i++) {
		struct plot_data *entry = pi->entry + i;
		if (!last_entry || last_entry->cylinderindex != entry->cylinderindex) {
			last = i;
			last_entry = entry;
			entry->sac = get_local_sac(entry, pi->entry + i + 1, dive);
		} else {
			int j;
			entry->sac = 0;
			for (j = last; j < i; j++)
				entry->sac += get_local_sac(pi->entry + j, pi->entry + j + 1, dive);
			entry->sac /= (i - last);
			if (entry->sec - last_entry->sec >= SAC_WINDOW) {
				last++;
				last_entry = pi->entry + last;
			}
		}
	}
}

static void populate_secondary_sensor_data(struct divecomputer *dc, struct plot_info *pi)
{
	/* We should try to see if it has interesting pressure data here */
}

static void setup_gas_sensor_pressure(struct dive *dive, struct divecomputer *dc, struct plot_info *pi)
{
	int i;
	struct divecomputer *secondary;

	/* First, populate the pressures with the manual cylinder data.. */
	for (i = 0; i < MAX_CYLINDERS; i++) {
		cylinder_t *cyl = dive->cylinder + i;
		int start = cyl->start.mbar ?: cyl->sample_start.mbar;
		int end = cyl->end.mbar ?: cyl->sample_end.mbar;

		if (!start || !end)
			continue;

		populate_cylinder_pressure_data(i, start, end, pi);
	}

	/*
	 * Here, we should try to walk through all the dive computers,
	 * and try to see if they have sensor data different from the
	 * primary dive computer (dc).
	 */
	secondary = &dive->dc;
	do {
		if (secondary == dc)
			continue;
		populate_secondary_sensor_data(dc, pi);
	} while ((secondary = secondary->next) != NULL);
}

static void populate_pressure_information(struct dive *dive, struct divecomputer *dc, struct plot_info *pi)
{
	int i, cylinderindex;
	pr_track_t *track_pr[MAX_CYLINDERS] = { NULL, };
	pr_track_t *current;
	bool missing_pr = false;

	cylinderindex = -1;
	current = NULL;
	for (i = 0; i < pi->nr; i++) {
		struct plot_data *entry = pi->entry + i;
		int pressure = SENSOR_PRESSURE(entry);

		/* discrete integration of pressure over time to get the SAC rate equivalent */
		if (current) {
			entry->pressure_time = pressure_time(dive, dc, entry - 1, entry);
			current->pressure_time += entry->pressure_time;
			current->t_end = entry->sec;
		}

		/* track the segments per cylinder and their pressure/time integral */
		if (entry->cylinderindex != cylinderindex) {
			cylinderindex = entry->cylinderindex;
			current = pr_track_alloc(pressure, entry->sec);
			track_pr[cylinderindex] = list_add(track_pr[cylinderindex], current);
			continue;
		}

		if (!pressure) {
			missing_pr = 1;
			continue;
		}

		current->end = pressure;

		/* Was it continuous? */
		if (SENSOR_PRESSURE(entry - 1))
			continue;

		/* transmitter changed its working status */
		current = pr_track_alloc(pressure, entry->sec);
		track_pr[cylinderindex] = list_add(track_pr[cylinderindex], current);
	}

	if (missing_pr) {
		fill_missing_tank_pressures(dive, pi, track_pr);
	}
	for (i = 0; i < MAX_CYLINDERS; i++)
		list_free(track_pr[i]);
}

/* calculate DECO STOP / TTS / NDL */
static void calculate_ndl_tts(double tissue_tolerance, struct plot_data *entry, struct dive *dive, double surface_pressure)
{
	/* FIXME: This should be configurable */
	/* ascent speed up to first deco stop */
	const int ascent_s_per_step = 1;
	const int ascent_mm_per_step = 200; /* 12 m/min */
	/* ascent speed between deco stops */
	const int ascent_s_per_deco_step = 1;
	const int ascent_mm_per_deco_step = 16; /* 1 m/min */
	/* how long time steps in deco calculations? */
	const int time_stepsize = 60;
	const int deco_stepsize = 3000;
	/* at what depth is the current deco-step? */
	int next_stop = ROUND_UP(deco_allowed_depth(tissue_tolerance, surface_pressure, dive, 1), deco_stepsize);
	int ascent_depth = entry->depth;
	/* at what time should we give up and say that we got enuff NDL? */
	const int max_ndl = 7200;
	int cylinderindex = entry->cylinderindex;

	/* If we don't have a ceiling yet, calculate ndl. Don't try to calculate
	 * a ndl for lower values than 3m it would take forever */
	if (next_stop == 0) {
		if (entry->depth < 3000) {
			entry->ndl = max_ndl;
			return;
		}
		/* stop if the ndl is above max_ndl seconds, and call it plenty of time */
		while (entry->ndl_calc < max_ndl && deco_allowed_depth(tissue_tolerance, surface_pressure, dive, 1) <= 0) {
			entry->ndl_calc += time_stepsize;
			tissue_tolerance = add_segment(depth_to_mbar(entry->depth, dive) / 1000.0,
						       &dive->cylinder[cylinderindex].gasmix, time_stepsize, entry->po2 * 1000, dive);
		}
		/* we don't need to calculate anything else */
		return;
	}

	/* We are in deco */
	entry->in_deco_calc = true;

	/* Add segments for movement to stopdepth */
	for (; ascent_depth > next_stop; ascent_depth -= ascent_mm_per_step, entry->tts_calc += ascent_s_per_step) {
		tissue_tolerance = add_segment(depth_to_mbar(ascent_depth, dive) / 1000.0,
					       &dive->cylinder[cylinderindex].gasmix, ascent_s_per_step, entry->po2 * 1000, dive);
		next_stop = ROUND_UP(deco_allowed_depth(tissue_tolerance, surface_pressure, dive, 1), deco_stepsize);
	}
	ascent_depth = next_stop;

	/* And how long is the current deco-step? */
	entry->stoptime_calc = 0;
	entry->stopdepth_calc = next_stop;
	next_stop -= deco_stepsize;

	/* And how long is the total TTS */
	while (next_stop >= 0) {
		/* save the time for the first stop to show in the graph */
		if (ascent_depth == entry->stopdepth_calc)
			entry->stoptime_calc += time_stepsize;

		entry->tts_calc += time_stepsize;
		tissue_tolerance = add_segment(depth_to_mbar(ascent_depth, dive) / 1000.0,
					       &dive->cylinder[cylinderindex].gasmix, time_stepsize, entry->po2 * 1000, dive);

		if (deco_allowed_depth(tissue_tolerance, surface_pressure, dive, 1) <= next_stop) {
			/* move to the next stop and add the travel between stops */
			for (; ascent_depth > next_stop; ascent_depth -= ascent_mm_per_deco_step, entry->tts_calc += ascent_s_per_deco_step)
				add_segment(depth_to_mbar(ascent_depth, dive) / 1000.0,
					    &dive->cylinder[cylinderindex].gasmix, ascent_s_per_deco_step, entry->po2 * 1000, dive);
			ascent_depth = next_stop;
			next_stop -= deco_stepsize;
		}
	}
}

/* Let's try to do some deco calculations.
 * Needs to be run before calculate_gas_information so we know that if we have a po2, where in ccr-mode.
 */
void calculate_deco_information(struct dive *dive, struct divecomputer *dc, struct plot_info *pi, bool print_mode)
{
	int i;
	double surface_pressure = (dc->surface_pressure.mbar ? dc->surface_pressure.mbar : get_surface_pressure_in_mbar(dive, true)) / 1000.0;
	double tissue_tolerance = 0;
	int last_ndl_tts_calc_time = 0;
	for (i = 1; i < pi->nr; i++) {
		struct plot_data *entry = pi->entry + i;
		int j, t0 = (entry - 1)->sec, t1 = entry->sec;
		int time_stepsize = 20;
		if (t0 != t1 && t1 - t0 < time_stepsize)
			time_stepsize = t1 - t0;
		for (j = t0 + time_stepsize; j <= t1; j += time_stepsize) {
			int depth = interpolate(entry[-1].depth, entry[0].depth, j - t0, t1 - t0);
			double min_pressure = add_segment(depth_to_mbar(depth, dive) / 1000.0,
							  &dive->cylinder[entry->cylinderindex].gasmix, time_stepsize, entry->po2 * 1000, dive);
			tissue_tolerance = min_pressure;
			if (j - t0 < time_stepsize)
				time_stepsize = j - t0;
		}
		if (t0 == t1)
			entry->ceiling = (entry - 1)->ceiling;
		else
			entry->ceiling = deco_allowed_depth(tissue_tolerance, surface_pressure, dive, !prefs.calcceiling3m);
		for (j = 0; j < 16; j++)
			entry->ceilings[j] = deco_allowed_depth(tolerated_by_tissue[j], surface_pressure, dive, 1);

		/* should we do more calculations?
		 * We don't for print-mode because this info doesn't show up there */
		if (prefs.calcndltts && !print_mode) {
			/* only calculate ndl/tts on every 30 seconds */
			if ((entry->sec - last_ndl_tts_calc_time) < 30) {
				struct plot_data *prev_entry = (entry - 1);
				entry->stoptime_calc = prev_entry->stoptime_calc;
				entry->stopdepth_calc = prev_entry->stopdepth_calc;
				entry->tts_calc = prev_entry->tts_calc;
				entry->ndl_calc = prev_entry->ndl_calc;
				continue;
			}
			last_ndl_tts_calc_time = entry->sec;

			/* We are going to mess up deco state, so store it for later restore */
			char *cache_data = NULL;
			cache_deco_state(tissue_tolerance, &cache_data);
			calculate_ndl_tts(tissue_tolerance, entry, dive, surface_pressure);
			/* Restore "real" deco state for next real time step */
			tissue_tolerance = restore_deco_state(cache_data);
			free(cache_data);
		}
	}
#if DECO_CALC_DEBUG & 1
	dump_tissues();
#endif
}

static void calculate_gas_information_new(struct dive *dive, struct plot_info *pi)
{
	int i;
	double amb_pressure;

	for (i = 1; i < pi->nr; i++) {
		int fo2, fhe;
		struct plot_data *entry = pi->entry + i;
		int cylinderindex = entry->cylinderindex;

		amb_pressure = depth_to_mbar(entry->depth, dive) / 1000.0;
		fo2 = get_o2(&dive->cylinder[cylinderindex].gasmix);
		fhe = get_he(&dive->cylinder[cylinderindex].gasmix);

		if (entry->po2) {
			/* we have an O₂ partial pressure in the sample - so this
			 * is likely a CC dive... use that instead of the value
			 * from the cylinder info */
			if (entry->po2 >= amb_pressure || fo2 == 1000) {
				entry->po2 = amb_pressure;
				entry->phe = 0;
				entry->pn2 = 0;
			} else {
				entry->phe = (amb_pressure - entry->po2) * (double)fhe / (1000 - fo2);
				entry->pn2 = amb_pressure - entry->po2 - entry->phe;
			}
		} else {
			entry->po2 = fo2 / 1000.0 * amb_pressure;
			entry->phe = fhe / 1000.0 * amb_pressure;
			entry->pn2 = (1000 - fo2 - fhe) / 1000.0 * amb_pressure;
		}

		/* Calculate MOD, EAD, END and EADD based on partial pressures calculated before
		 * so there is no difference in calculating between OC and CC
		 * END takes O₂ + N₂ (air) into account ("Narcotic" for trimix dives)
		 * EAD just uses N₂ ("Air" for nitrox dives) */
		pressure_t modpO2 = { .mbar = (int) (prefs.modpO2 * 1000) };
		entry->mod = (double) gas_mod(&dive->cylinder[cylinderindex].gasmix, modpO2, 1).mm;
		entry->end = (entry->depth + 10000) * (1000 - fhe) / 1000.0 - 10000;
		entry->ead = (entry->depth + 10000) * (1000 - fo2 - fhe) / (double)N2_IN_AIR - 10000;
		entry->eadd = (entry->depth + 10000) *
				  (entry->po2 / amb_pressure * O2_DENSITY + entry->pn2 / amb_pressure *
										N2_DENSITY +
				   entry->phe / amb_pressure * HE_DENSITY) /
				  (O2_IN_AIR * O2_DENSITY + N2_IN_AIR * N2_DENSITY) * 1000 - 10000;
		if (entry->mod < 0)
			entry->mod = 0;
		if (entry->ead < 0)
			entry->ead = 0;
		if (entry->end < 0)
			entry->end = 0;
		if (entry->eadd < 0)
			entry->eadd = 0;
	}
}
/*
 * Create a plot-info with smoothing and ranged min/max
 *
 * This also makes sure that we have extra empty events on both
 * sides, so that you can do end-points without having to worry
 * about it.
 */
void create_plot_info_new(struct dive *dive, struct divecomputer *dc, struct plot_info *pi)
{
	int o2, he, o2low;
	init_decompression(dive);
	/* Create the new plot data */
	free((void *)last_pi_entry_new);
	get_dive_gas(dive, &o2, &he, &o2low);
	if (he > 0) {
		pi->dive_type = TRIMIX;
	} else {
		if (o2)
			pi->dive_type = NITROX;
		else
			pi->dive_type = AIR;
	}
	last_pi_entry_new = populate_plot_entries(dive, dc, pi);
	check_gas_change_events(dive, dc, pi);       /* Populate the gas index from the gas change events */
	setup_gas_sensor_pressure(dive, dc, pi);     /* Try to populate our gas pressure knowledge */
	populate_pressure_information(dive, dc, pi); /* .. calculate missing pressure entries */
	calculate_sac(dive, pi);		     /* Calculate sac */
	calculate_deco_information(dive, dc, pi, false);
	calculate_gas_information_new(dive, pi); /* And finaly calculate gas partial pressures */
	pi->meandepth = dive->dc.meandepth.mm;
	analyze_plot_info(pi);
}

struct divecomputer *select_dc(struct dive *dive)
{
	unsigned int max = number_of_computers(dive);
	unsigned int i = dc_number;

	/* Reset 'dc_number' if we've switched dives and it is now out of range */
	if (i >= max)
		dc_number = i = 0;

	return get_dive_dc(dive, i);
}

static void plot_string(struct plot_info *pi, struct plot_data *entry, struct membuffer *b, bool has_ndl)
{
	int pressurevalue, mod, ead, end, eadd;
	const char *depth_unit, *pressure_unit, *temp_unit, *vertical_speed_unit;
	double depthvalue, tempvalue, speedvalue, sacvalue;
	int decimals;
	const char *unit;

	depthvalue = get_depth_units(entry->depth, NULL, &depth_unit);
	put_format(b, translate("gettextFromC", "@: %d:%02d\nD: %.1f%s\n"), FRACTION(entry->sec, 60), depthvalue, depth_unit);
	if (GET_PRESSURE(entry)) {
		pressurevalue = get_pressure_units(GET_PRESSURE(entry), &pressure_unit);
		put_format(b, translate("gettextFromC", "P: %d%s\n"), pressurevalue, pressure_unit);
	}
	if (entry->temperature) {
		tempvalue = get_temp_units(entry->temperature, &temp_unit);
		put_format(b, translate("gettextFromC", "T: %.1f%s\n"), tempvalue, temp_unit);
	}
	speedvalue = get_vertical_speed_units(abs(entry->speed), NULL, &vertical_speed_unit);
	/* Ascending speeds are positive, descending are negative */
	if (entry->speed > 0)
		speedvalue *= -1;
	put_format(b, translate("gettextFromC", "V: %.1f%s\n"), speedvalue, vertical_speed_unit);
	sacvalue = get_volume_units(entry->sac, &decimals, &unit);
	if (entry->sac && prefs.show_sac)
		put_format(b, translate("gettextFromC", "SAC: %.*f%s/min\n"), decimals, sacvalue, unit);
	if (entry->cns)
		put_format(b, translate("gettextFromC", "CNS: %u%%\n"), entry->cns);
	if (prefs.pp_graphs.po2)
		put_format(b, translate("gettextFromC", "pO%s: %.2fbar\n"), UTF8_SUBSCRIPT_2, entry->po2);
	if (prefs.pp_graphs.pn2)
		put_format(b, translate("gettextFromC", "pN%s: %.2fbar\n"), UTF8_SUBSCRIPT_2, entry->pn2);
	if (prefs.pp_graphs.phe)
		put_format(b, translate("gettextFromC", "pHe: %.2fbar\n"), entry->phe);
	if (prefs.mod) {
		mod = (int)get_depth_units(entry->mod, NULL, &depth_unit);
		put_format(b, translate("gettextFromC", "MOD: %d%s\n"), mod, depth_unit);
	}
	eadd = (int)get_depth_units(entry->eadd, NULL, &depth_unit);
	if (prefs.ead) {
		switch (pi->dive_type) {
		case NITROX:
			ead = (int)get_depth_units(entry->ead, NULL, &depth_unit);
			put_format(b, translate("gettextFromC", "EAD: %d%s\nEADD: %d%s\n"), ead, depth_unit, eadd, depth_unit);
			break;
		case TRIMIX:
			end = (int)get_depth_units(entry->end, NULL, &depth_unit);
			put_format(b, translate("gettextFromC", "END: %d%s\nEADD: %d%s\n"), end, depth_unit, eadd, depth_unit);
			break;
		case AIR:
			/* nothing */
			break;
		}
	}
	if (entry->stopdepth) {
		depthvalue = get_depth_units(entry->stopdepth, NULL, &depth_unit);
		if (entry->ndl) {
			/* this is a safety stop as we still have ndl */
			if (entry->stoptime)
				put_format(b, translate("gettextFromC", "Safetystop: %umin @ %.0f%s\n"), DIV_UP(entry->stoptime, 60),
					   depthvalue, depth_unit);
			else
				put_format(b, translate("gettextFromC", "Safetystop: unkn time @ %.0f%s\n"),
					   depthvalue, depth_unit);
		} else {
			/* actual deco stop */
			if (entry->stoptime)
				put_format(b, translate("gettextFromC", "Deco: %umin @ %.0f%s\n"), DIV_UP(entry->stoptime, 60),
					   depthvalue, depth_unit);
			else
				put_format(b, translate("gettextFromC", "Deco: unkn time @ %.0f%s\n"),
					   depthvalue, depth_unit);
		}
	} else if (entry->in_deco) {
		put_string(b, translate("gettextFromC", "In deco\n"));
	} else if (has_ndl) {
		put_format(b, translate("gettextFromC", "NDL: %umin\n"), DIV_UP(entry->ndl, 60));
	}
	if (entry->tts)
		put_format(b, translate("gettextFromC", "TTS: %umin\n"), DIV_UP(entry->tts, 60));
	if (entry->stopdepth_calc && entry->stoptime_calc) {
		depthvalue = get_depth_units(entry->stopdepth_calc, NULL, &depth_unit);
		put_format(b, translate("gettextFromC", "Deco: %umin @ %.0f%s (calc)\n"), DIV_UP(entry->stoptime_calc, 60),
			   depthvalue, depth_unit);
	} else if (entry->in_deco_calc) {
		/* This means that we have no NDL left,
		 * and we have no deco stop,
		 * so if we just accend to the surface slowly
		 * (ascent_mm_per_step / ascent_s_per_step)
		 * everything will be ok. */
		put_string(b, translate("gettextFromC", "In deco (calc)\n"));
	} else if (prefs.calcndltts && entry->ndl_calc != 0) {
		put_format(b, translate("gettextFromC", "NDL: %umin (calc)\n"), DIV_UP(entry->ndl_calc, 60));
	}
	if (entry->tts_calc)
		put_format(b, translate("gettextFromC", "TTS: %umin (calc)\n"), DIV_UP(entry->tts_calc, 60));
	if (entry->ceiling) {
		depthvalue = get_depth_units(entry->ceiling, NULL, &depth_unit);
		put_format(b, translate("gettextFromC", "Calculated ceiling %.0f%s\n"), depthvalue, depth_unit);
		if (prefs.calcalltissues) {
			int k;
			for (k = 0; k < 16; k++) {
				if (entry->ceilings[k]) {
					depthvalue = get_depth_units(entry->ceilings[k], NULL, &depth_unit);
					put_format(b, translate("gettextFromC", "Tissue %.0fmin: %.0f%s\n"), buehlmann_N2_t_halflife[k], depthvalue, depth_unit);
				}
			}
		}
	}
	if (entry->heartbeat && prefs.hrgraph)
		put_format(b, translate("gettextFromC", "heartbeat: %d\n"), entry->heartbeat);
	if (entry->bearing)
		put_format(b, translate("gettextFromC", "bearing: %d\n"), entry->bearing);
	strip_mb(b);
}

void get_plot_details_new(struct plot_info *pi, int time, struct membuffer *mb)
{
	struct plot_data *entry = NULL;
	int i;

	for (i = 0; i < pi->nr; i++) {
		entry = pi->entry + i;
		if (entry->sec >= time)
			break;
	}
	if (entry)
		plot_string(pi, entry, mb, pi->has_ndl);
}

/* Compare two plot_data entries and writes the results into a string */
void compare_samples(struct plot_data *e1, struct plot_data *e2, char *buf, int bufsize, int sum)
{
	struct plot_data *start, *stop, *data;
	const char *depth_unit, *pressure_unit, *vertical_speed_unit;
	char *buf2 = malloc(bufsize);
	int avg_speed, max_asc_speed, max_desc_speed;
	int delta_depth, avg_depth, max_depth, min_depth;
	int bar_used, last_pressure, pressurevalue;
	int count, last_sec, delta_time;

	double depthvalue, speedvalue;

	if (bufsize > 0)
		buf[0] = '\0';
	if (e1 == NULL || e2 == NULL) {
		free(buf2);
		return;
	}

	if (e1->sec < e2->sec) {
		start = e1;
		stop = e2;
	} else if (e1->sec > e2->sec) {
		start = e2;
		stop = e1;
	} else {
		free(buf2);
		return;
	}
	count = 0;
	avg_speed = 0;
	max_asc_speed = 0;
	max_desc_speed = 0;

	delta_depth = abs(start->depth - stop->depth);
	delta_time = abs(start->sec - stop->sec);
	avg_depth = 0;
	max_depth = 0;
	min_depth = INT_MAX;
	bar_used = 0;

	last_sec = start->sec;
	last_pressure = GET_PRESSURE(start);

	data = start;
	while (data != stop) {
		data = start + count;
		if (sum)
			avg_speed += abs(data->speed) * (data->sec - last_sec);
		else
			avg_speed += data->speed * (data->sec - last_sec);
		avg_depth += data->depth * (data->sec - last_sec);

		if (data->speed > max_desc_speed)
			max_desc_speed = data->speed;
		if (data->speed < max_asc_speed)
			max_asc_speed = data->speed;

		if (data->depth < min_depth)
			min_depth = data->depth;
		if (data->depth > max_depth)
			max_depth = data->depth;
		/* Try to detect gas changes */
		if (GET_PRESSURE(data) < last_pressure + 2000)
			bar_used += last_pressure - GET_PRESSURE(data);

		count += 1;
		last_sec = data->sec;
		last_pressure = GET_PRESSURE(data);
	}
	avg_depth /= stop->sec - start->sec;
	avg_speed /= stop->sec - start->sec;

	snprintf(buf, bufsize, translate("gettextFromC", "%sT: %d:%02d min"), UTF8_DELTA, delta_time / 60, delta_time % 60);
	memcpy(buf2, buf, bufsize);

	depthvalue = get_depth_units(delta_depth, NULL, &depth_unit);
	snprintf(buf, bufsize, translate("gettextFromC", "%s %sD:%.1f%s"), buf2, UTF8_DELTA, depthvalue, depth_unit);
	memcpy(buf2, buf, bufsize);

	depthvalue = get_depth_units(min_depth, NULL, &depth_unit);
	snprintf(buf, bufsize, translate("gettextFromC", "%s %sD:%.1f%s"), buf2, UTF8_DOWNWARDS_ARROW, depthvalue, depth_unit);
	memcpy(buf2, buf, bufsize);

	depthvalue = get_depth_units(max_depth, NULL, &depth_unit);
	snprintf(buf, bufsize, translate("gettextFromC", "%s %sD:%.1f%s"), buf2, UTF8_UPWARDS_ARROW, depthvalue, depth_unit);
	memcpy(buf2, buf, bufsize);

	depthvalue = get_depth_units(avg_depth, NULL, &depth_unit);
	snprintf(buf, bufsize, translate("gettextFromC", "%s %sD:%.1f%s\n"), buf2, UTF8_AVERAGE, depthvalue, depth_unit);
	memcpy(buf2, buf, bufsize);

	speedvalue = get_vertical_speed_units(abs(max_desc_speed), NULL, &vertical_speed_unit);
	snprintf(buf, bufsize, translate("gettextFromC", "%s%sV:%.2f%s"), buf2, UTF8_DOWNWARDS_ARROW, speedvalue, vertical_speed_unit);
	memcpy(buf2, buf, bufsize);

	speedvalue = get_vertical_speed_units(abs(max_asc_speed), NULL, &vertical_speed_unit);
	snprintf(buf, bufsize, translate("gettextFromC", "%s %sV:%.2f%s"), buf2, UTF8_UPWARDS_ARROW, speedvalue, vertical_speed_unit);
	memcpy(buf2, buf, bufsize);

	speedvalue = get_vertical_speed_units(abs(avg_speed), NULL, &vertical_speed_unit);
	snprintf(buf, bufsize, translate("gettextFromC", "%s %sV:%.2f%s"), buf2, UTF8_AVERAGE, speedvalue, vertical_speed_unit);
	memcpy(buf2, buf, bufsize);

	/* Only print if gas has been used */
	if (bar_used) {
		pressurevalue = get_pressure_units(bar_used, &pressure_unit);
		memcpy(buf2, buf, bufsize);
		snprintf(buf, bufsize, translate("gettextFromC", "%s %sP:%d %s"), buf2, UTF8_DELTA, pressurevalue, pressure_unit);
	}

	free(buf2);
}
