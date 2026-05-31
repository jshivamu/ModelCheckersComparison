#include <stdio.h>
#include <stdlib.h>
//#include <stdbool.h>
#include <assert.h>
#include <time.h>

/* Constants based on requirements */
#define CONTROL_LOOP_PERIOD_MS 10
#define TICKS_PER_SECOND 100
#define MAX_THROTTLE 100.0f
#define MIN_THROTTLE 40.0f
#define MAX_THROTTLE_RATE_PER_SEC 10.0f
#define STALL_SPEED 30.0f

/* Flight Modes */
typedef enum {
    NORMAL,
    CLIMB,
    STALL_RECOVERY
} FlightMode;

/* Global State Variables */
float actual_alt, ref_alt;
float actual_speed, ref_speed;
float current_pitch, current_throttle, previous_throttle;
FlightMode current_mode = NORMAL;
FlightMode prev_mode = NORMAL;

/* Persistence Counters for 1-second requirements */
int climb_trigger_cnt = 0;
int climb_exit_cnt = 0;
int stall_trigger_cnt = 0;
int stall_exit_cnt = 0;

//int srand_initialized = 0;

/* External Control Loop Logic (Placeholders) */
float compute_altitude_loop_pitch() {
    return (ref_alt - actual_alt) * 0.5f;
}
float compute_speed_loop_throttle() {
    return 60.0f; /* Simplified base throttle */
}

void step_control_computer() {
	previous_throttle = current_throttle;

    /* 1. Update Persistence Counters */
    if (ref_alt > (actual_alt + 100.0f)) climb_trigger_cnt++; else climb_trigger_cnt = 0;
    if (actual_alt >= ref_alt) climb_exit_cnt++; else climb_exit_cnt = 0;
    if (actual_speed < STALL_SPEED) stall_trigger_cnt++; else stall_trigger_cnt = 0;
    if (actual_speed > (STALL_SPEED + 5.0f)) stall_exit_cnt++; else stall_exit_cnt = 0;

    /* 2. Mode Priority Logic (Stall Recovery > Climb > Normal) */
    if (stall_trigger_cnt >= TICKS_PER_SECOND) {
        current_mode = STALL_RECOVERY;
    } else if (current_mode == STALL_RECOVERY && stall_exit_cnt >= TICKS_PER_SECOND) {
        if(climb_trigger_cnt >= TICKS_PER_SECOND )
            current_mode = CLIMB;
        else
            current_mode = NORMAL;
    } else if (climb_trigger_cnt >= TICKS_PER_SECOND && current_mode != STALL_RECOVERY) {
        current_mode = CLIMB;
    } else if (current_mode == CLIMB && climb_exit_cnt >= TICKS_PER_SECOND) {
        current_mode = NORMAL;
    }

    /* 3. Execute Mode-Specific Logic */
    float target_pitch, target_throttle;

    if (current_mode == STALL_RECOVERY) {
        float alt_pitch = compute_altitude_loop_pitch();
        target_pitch = (alt_pitch < 0.0f) ? alt_pitch : 0.0f;
        target_throttle = MAX_THROTTLE;
    } else if (current_mode == CLIMB) {
        target_pitch = 10.0f;
        target_throttle = MAX_THROTTLE;
    } else {
        target_pitch = compute_altitude_loop_pitch();
        target_throttle = compute_speed_loop_throttle();
    }

    /* 4. Apply Physical Constraints */
    if (target_throttle > MAX_THROTTLE) target_throttle = MAX_THROTTLE;
    if (target_throttle < MIN_THROTTLE) target_throttle = MIN_THROTTLE;

    /* Rate Limiter: 10% per second = 0.1% per 10ms tick */
    float max_increase = MAX_THROTTLE_RATE_PER_SEC / (float)TICKS_PER_SECOND;
    if (target_throttle > (current_throttle + max_increase)) {
        current_throttle = current_throttle + max_increase;
    } else if (target_throttle < (current_throttle - max_increase)) {
        current_throttle = current_throttle - max_increase;
    } else {
        current_throttle = target_throttle;
    }

    current_pitch = target_pitch;
}

void get_externl_inputs()
{
    int random;

    //if(!srand_initialized)
    //{
    //    srand(time(NULL));
    //    srand_initialized = 1;
    //}

    random = nondet_int();
	__CPROVER_assume(random >= 0 && random <= 100);
    if(random%2)
        actual_alt = actual_alt - ((random)*0.1f);
    else
        actual_alt = actual_alt + ((random)*0.1f);

    //random = rand();
    if(random%2)
        actual_speed = actual_speed - ((random)*0.001f);
    else
        actual_speed = actual_speed + ((random)*0.001f);

}

int main()
{
    int counter = 0;
    current_throttle = 60.0f;
	ref_alt = 5000.0f;
	ref_speed = 50.0f;

	actual_alt = 4950.0f; //Initial value assumed
	actual_speed = 40.0f;

	while(counter < 1000)
	{
	    prev_mode = current_mode;
	    get_externl_inputs();
        step_control_computer();

		if(current_mode != prev_mode)
        //if(!(counter%10))
        //{
        //    printf("Mode : %d, speed %f, alt %f\n", current_mode, //actual_speed, actual_alt);

        //}

        counter++;

		//Bunch of asserts
		assert(current_throttle >= 40.0 && current_throttle <= 100.0);
		assert(abs(current_throttle - previous_throttle) <= 0.101);
		if (stall_trigger_cnt >= 100) assert(current_mode == STALL_RECOVERY);
		if((current_mode != STALL_RECOVERY) &&
			(climb_trigger_cnt >= TICKS_PER_SECOND))
			assert(current_mode == CLIMB);

		if (current_mode == CLIMB) assert(current_pitch == 10.0);
		if (current_mode == STALL_RECOVERY) assert(current_pitch <= 0.0);

	}

    return 0;
}
