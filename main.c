#include "theheader.h"

int AIMBOT          = 0;
int INFINITE_HEALTH = 0;
input_t input_type = MANUAL;

SDL_Window *screen;
SDL_Renderer *renderer;
TTF_Font *font;

int wall_size = 100;

float max_thrust = 0.7;
float max_velocity = 10;
float thrust = .05;
float slow = .975;

float rotation_speed = 0.1f;

int maxx = 0;
int maxy = 0;
int maxz = 0;

int outpost_health = 200;
int base_health = 1000;

int minimap_scale;
int minimap_size;

int bullet_damage = 1;

struct keyboard_s keyboard;
SDL_Event event;


int main(int argc, char **argv)
{

	/*
	* Declarations
	*/
	unsigned int frame_limit, ticks;
	struct cam_s camera;

	struct thread_params_s params;

	pthread_t thread[1];

	entity *players = NULL;
	entity *ai = NULL;
	entity *entities, *c, *c2;
	entity *bullet;

	if (argc > 1)
		printf("Usage: %s\n", argv[0]);

	/*
	 * Initialisation
	 */


	/* Setup SDL */
	if (SDL_Init(SDL_INIT_TIMER) < 0)
	{
		printf("Failed to init SDL: %s\n", SDL_GetError());
		return 1;
	}
	atexit(cleanup);




	/* Init data */
	memset(&(keyboard), 0, sizeof(struct keyboard_s));
	memset(&(camera), 0, sizeof(struct cam_s));

	entities = load_entities("maps/rush.dat");

	players = (entity *)malloc(sizeof(entity));
	memset(players, 0, sizeof(entity));
	players->next = NULL;

	bullet = (entity *)malloc(sizeof(entity));
	memset(bullet, 0, sizeof(entity));
	bullet->type = TRASH;
	bullet->next = NULL;


	/* Add players */
	for (c = entities;; c = c->next)
	{
		if (c == NULL || c->next == NULL)
			break;
		if (c->type == BASE)
			players = add_player(entities, players, 1);

	}

	/* Init Misc */
	srand(time(NULL));




	/*
	 * Initialise the multiplayer breads
	 */
	params.players = players;
	pthread_create(&thread[0], NULL, update_players, &params);


	/*
	* Loop
	*/
	frame_limit = SDL_GetTicks() + 16;
	while (1)
	{
		/*
		* Physics
		*/
		for (c = players;; c = c->next)
		{
			if (c == NULL || c->next == NULL)
				break;
			if (c->type == TRASH)
				continue;

			if ((c2 = check_bullet_collision(bullet, c)) != NULL)
				c->health -= c2->damage;

			if (c->health <= 0)
			{
				if (INFINITE_HEALTH)
					c->health = 1000;
				else
					respawn(entities, c, players);
				get_entity_by_num(players, c2->team)->xp += 50;
			}

			if (c->type == P_AI)
			{
				c->angle = angle_to_entity((c2 = find_closest_enemy(c, players, 800,
					   entities)), c);

				if (c->weapon < SHOTGUN_2)
				{
					if (c2 == c)
						c->angle = angle_to_entity((c2 = find_closest_enemy(c, entities, maxx+maxy,
						entities)), c);
					if (c2 == c)
						c->angle = angle_to_entity(find_nearest_type(entities, c, LIFT,
						                                             DROP), c);
				}
				else
				{
					c->angle = angle_to_entity((c2 = find_nearest_type(entities, c, BASE, BASE)), c);
					if (c2 == c)
						c->angle = angle_to_entity(find_nearest_type(entities, c, LIFT,
						DROP), c);
				}


				/*c->thrust = (sqrt(pow((float)c->x - c2->x, 2) + pow((float)c->y - c2->y, 2))) / ((rand() % 10) + 10);
				if (c->thrust > max_thrust || c->thrust < 0)
				c->thrust = max_thrust;*/


				c->thrust = max_thrust;


				c->t_angle = angle_to_entity((c2 = find_closest_enemy(c, players, 1000, entities)),
					c);

				if (c2 == c)
					c->t_angle = angle_to_entity((c2 = find_closest_enemy(c, ai, 1000,
					entities)), c);

				if (c2 == c)
					c->t_angle = angle_to_entity((c2 = find_closest_enemy(c,
					entities, 1000, entities)), c);

				c->cool_pos--;
				if (c2 != c)
					fire_weapon(bullet, c);
			}


			update_entity_collision(entities, c);

			update_velocity(c);
			update_aura_v(entities, c);
			update_wall_aura_v(c);

			c->x = get_next_x(c);
			c->y = get_next_y(c);

			c->thrust *= slow;
			c->xv *= slow;
			c->yv *= slow;

			if (c->xp >= 100)
				level_up(c);
		}
		/* Move the ai lackeys */
		for (c = ai;; c = c->next)
		{
			if (c == NULL || c->next == NULL)
				break;
			if (c->type == TRASH)
				continue;



			if ((c2 = check_bullet_collision(bullet, c)) != NULL)
				c->health -= c2->damage;

			if (c->health <= 0)
			{
				respawn(entities, c, players);
				find_player(players, c2->team)->xp += 10;
			}


			set_squad_position(entities, c, find_player(players, c->team), ai);
			switch (c->mode)
			{
				default:
				case BEHIND:
					if (c->mode != TARGET)
						c->t_angle = angle_to_entity((c2 = find_closest_enemy(c,
						ai, 1000, entities)), c);
					if (c2 == c && c->mode != TARGET)
						c->t_angle = angle_to_entity((c2 = find_closest_enemy(c,
						players, 1000, entities)), c);
					if (c2 == c && c->mode != TARGET)
						c->t_angle = angle_to_entity((c2 = find_closest_enemy(c,
						entities, 5000, entities)), c);

					c->thrust = (sqrt(pow((float)c->x - c2->x, 2) +
						pow((float)c->y - c2->y, 2))) / ((rand() % 10) + 10);
					if (c->thrust > max_thrust - 2)
						c->thrust = max_thrust;
					if (c2 != c && c->mode != TARGET)
						fire_weapon(bullet, c);
					break;
				case DEPLOY:
					c->t_angle = angle_to_entity((c2 = find_closest_enemy(c, ai,
						1000, entities)), c);
					if (c2 == c)
						c->t_angle = angle_to_entity((c2 = find_closest_enemy(c,
						players, 1000, entities)), c);
					if (c2 == c)
						c->t_angle = angle_to_entity((c2 = find_closest_enemy(c,
						entities, 1000, entities)), c);
					if (keyboard.space || keyboard.up)
						fire_weapon(bullet, c);
					c->thrust = 0;
					break;

			}
			c->cool_pos--;


			update_entity_collision(entities, c);


			update_velocity(c);
			update_aura_v(players, c);
			update_wall_aura_v(c);
			c->xv *= slow - 0.001;
			c->yv *= slow - 0.001;
			c->x = get_next_x(c);
			c->y = get_next_y(c);
		}
		/* Move the bulllets */
		for (c = bullet;; c = c->next)
		{
			if (c == NULL || c->next == NULL)
				break;
			if (c->type == TRASH)
				continue;


			bullet_damage = c->damage;
			for (c2 = entities;; c2 = c2->next)
			{
				if (c2 == NULL || c2->next == NULL)
					break;
				if (test_collision_square(c, c2) == OTHER)
				{
					if (c2->type == L_TURRET &&
						c->team == c2->team)
						continue;
					c->type = TRASH;
					if (c2->type == OUTPOST && c2->team != c->team &&
					    owns_outpost_level(entities, c->team, c2->id - 1))
					{
						c2->health -= bullet_damage;
						if (c2->health <= 0)
						{
							c2->health = outpost_health;
							c2->team = c->team;
							if (get_lackey_count(ai, c->team) <
							    find_player(players, c->team)->lackey_cap)
							{
								ai = add_ai(entities, ai, 2, c->team,
								c2->weapon, c2->id2);
								find_player(players,
								c->team)->lackey_count += 2;
							}
							find_player(players, c->team)->xp += 30;
							find_player(players, c->team)->scrap += 100;
						}
					}
					if (c2->type == BASE && c2->team != c->team)
					{
						c2->health -= bullet_damage;
						if (c2->health <= 0)
						{
							/*c2->type = TRASH;*/
							c2->team = c->team;
							c2->health = base_health;;
						}
					}
					if (c2->type == L_TURRET)
					{
						c2->health -= bullet_damage;
						if (c2->health <= 0)
						{
							c2->type = DEAD_OUTPOST;
							c2->team = -9;
						}
					}
					break;
				}
			}
			c->age++;
			if (c->age > 50)
				c->type = TRASH;

			update_velocity(c);
			c->x = get_next_x(c);
			c->y = get_next_y(c);
		}

		/* Loop through the entities looking for turrets */
		for (c = entities;; c = c->next)
		{
			if (c == NULL || c->next == NULL)
				break;
			switch (c->type)
			{
				case L_TURRET:
					c->cool_pos--;

					c->angle = angle_to_entity((c2 = find_closest_enemy(c, players,
						1000, entities)), c);
					c->t_angle = angle_to_entity((c2 = find_closest_enemy(c, players,
						1000, entities)), c);
					if (c2 == c)
					{
						c->angle = angle_to_entity((c2 = find_closest_enemy(c,
						ai, 1000, entities)), c);
						c->t_angle = angle_to_entity((c2 = find_closest_enemy(c,
						ai, 1000, entities)), c);
					}
					if (c2 != c)
						fire_weapon(bullet, c);
					break;
				default:
					break;
			}
		}

		/*
		* Sleep
		*/
		if (frame_limit > (ticks = SDL_GetTicks()) + 16)
			SDL_Delay(16);
		else if (frame_limit > ticks)
			SDL_Delay(frame_limit - ticks);
		frame_limit = SDL_GetTicks() + 16;



	}

	return 0;
}

void cleanup()
{
	SDL_Quit();
}

int get_next_x(entity *e)
{
	return e->x + e->xv;
}
int get_next_y(entity *e)
{
	return e->y + e->yv;
}

void level_up(entity *player)
{
	switch (player->weapon)
	{
		default:
			break;
		case BASE_1:
			if (player->xp < 100)
				break;
			player->weapon = BASE_2;
			player->xp -= 100;
			break;
		case BASE_2:
			if (player->xp >= 150)
			{
				player->weapon = SHOTGUN_1;
				player->xp -= 150;
			}
			break;
		case SHOTGUN_1:
			if (player->xp >= 300)
			{
				player->weapon = SHOTGUN_2;
				player->xp -= 300;
			}
			break;
		case SHOTGUN_2:
			if (player->xp >= 400)
			{
				player->weapon = CRESCENT_1;
				player->xp -= 400;
			}
			break;
		case CRESCENT_1:
			if (player->xp >= 750)
			{
				player->weapon = CRESCENT_2;
				player->xp -= 750;
			}
			break;
	}

}

int get_level_health(e_form level, e_type type)
{
	if (type == PLAYER || type == P_AI)
		switch (level)
		{
			default:
			case BASE_1:
			case BASE_2:
					return 200;
			case SHOTGUN_1:
				return 250;
			case SHOTGUN_2:
				return 275;
			case CRESCENT_1:
				return 325;
			case CRESCENT_2:
				return 400;
		}
	else
		switch (level)
		{
			default:
			case BASE_1:
				return 15;
			case BASE_2:
				return 20;
			case SHOTGUN_1:
			case SHOTGUN_2:
				return 15;
			case CRESCENT_1:
				return 50;
			case CRESCENT_2:
				return 100;
		}
}

void respawn(entity *entities, entity *e, entity *players)
{
	entity *c;

	c = find_respawn_point(entities, e);

	if (e == c || (e->type == F_AI &&
	    !owns_outpost(entities, e->team, e->id2)))
	{
		e->type = TRASH;
		find_player(players, e->team)->lackey_count--;
		return;
	}

	e->x = c->x - (c->size);
	e->y = c->y - (c->size);
	e->z = c->z;
	e->health = get_level_health(e->weapon, e->type);
	e->xp = 0;
}



void update_entity_collision(entity *entities, entity *player)
{
	entity *c;
	for (c = entities;; c = c->next)
	{
		if (c == NULL || c->next == NULL)
			break;
		if (c->type == TRASH || c->z != player->z)
			continue;

		switch (c->type)
		{
			default:
				break;
			case DROP:
				if (test_collision_square(player, c) == OTHER)
					player->z--;
				break;
			case LIFT:
				if (test_collision_square(player, c) == OTHER)
					player->z++;
				break;
		}
	}
}
