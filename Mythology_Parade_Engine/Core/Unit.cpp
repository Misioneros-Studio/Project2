#include "Unit.h"
#include "p2Log.h"
#include "j1Textures.h"
#include "j1Input.h"
#include"CombatUnit.h"
#include "j1Gui.h"

Unit::Unit(UnitType type, iPoint pos): unitType(type), state(AnimationType::IDLE),  moveSpeed(60)
{
	
	if (App->entityManager->getPlayer())
	{
		displayDebug = App->entityManager->getPlayer()->displayDebug;
	}
	else
	{
		displayDebug = false;
	}
	collisionRect = { 0, 0, 30, -55 };
	unitType = type;
	position = {(float)pos.x, (float)pos.y};
	state = AnimationType::IDLE;
	flipState = SDL_FLIP_NONE;
	directionToTarget = {0, 0};
	normalizedDirection = { 0, 0 };
	timeToDespawn = 1.f;
	currentDirection = Direction::UP;
	//Init Units
	switch (type)
	{
	case UnitType::MONK:
		time_production = 90;
		time_research = 0;
		researched = true;
		Init(1);
		break;
	}
	SetSelected(false);

}

Unit::~Unit()
{
}

bool Unit::Start()
{
	bool ret = true;

	//pugi::xml_document	character_file;
	//pugi::xml_parse_result result = character_file.load_file("assets/units/Assassin.tmx");
	//pugi::xml_node	character_node = character_file.child("map");
	//std::string image_source("assets/units/");
	//image_source += character_node.child("tileset").child("image").attribute("source").as_string();
	//texture = App->tex->Load(image_source.c_str());
	//position_rect.w = 125;
	//position_rect.h = 112;
	//LOG("%s", image_source.c_str());
	combat_unit = false;
	show_bar_for_damage = false;
	return ret;
}


bool Unit::Update(float dt)
{
	bool ret = true;

	//Allawys blit the sprite at the end
	StateMachineActions(dt);
	//ret = Draw(dt);

	//Return
	int x, y;
	App->input->GetMousePosition(x, y);
	iPoint point = App->render->ScreenToWorld(x, y);
	if (App->scene->paused_game && show_bar_for_damage == true && damage_timer.isPaused() == false)
		damage_timer.Pause();
	else if (damage_timer.isPaused() == true && App->scene->paused_game == false)
		damage_timer.Resume();
	if (damaged_now == true) {
		damage_timer.Start();
		damaged_now = false;
		show_bar_for_damage = true;
	}
	if (show_bar_for_damage == true && damage_timer.ReadSec() > 2)
		show_bar_for_damage = false;
	if (isSelected() || (point.x >= collisionRect.x && point.x <= collisionRect.x + collisionRect.w && point.y <= collisionRect.y && point.y >= collisionRect.y + collisionRect.h) ||
		show_bar_for_damage == true) {
		if (civilization != App->entityManager->getPlayer()->civilization)
			Draw_Life_Bar(true);
		else
			Draw_Life_Bar();
	}
	return ret;
}

void Unit::SetMoveSpeed(int value)
{
	moveSpeed = value;
}


void Unit::MoveToTarget()
{
	//if (!isSelected())
	//	return;
	float speed = moveSpeed * App->GetDT();

	//Fast fix for ft increasing bug
	if (App->GetDT() >= 0.5f) 
	{
		speed = 0.f;
	}

  
	state = AnimationType::WALK;
	if (Mix_Playing(3) == 0) 
	{
		App->entityManager->FxUnits(3, App->entityManager->Walking_troops, position.x, position.y);
	}

	iPoint targetIso = App->map->MapToWorld(targetPosition.x, targetPosition.y);
	targetIso += App->map->GetTilesHalfSize();

	//App->render->DrawLine(position.x, position.y, targetIso.x, targetIso.y, 255, 0, 0);

	fPoint cast = { (float)targetIso.x, (float)targetIso.y };

	fPoint increment = { normalizedDirection.x * speed,  normalizedDirection.y * speed };

	position = position + increment;

	//state = AnimationType::WALK;

	if (position.DistanceManhattan(cast) <= 5)
	{
		position = App->map->MapToWorld((float)targetPosition.x, (float)targetPosition.y);
		position += App->map->GetTilesHalfSizeFloat();

		targetPosition.ResetAsPosition();
		if (entPath.size() <= 0)
		{
			if (enemyTarget != nullptr && unitType != UnitType::MONK) 
			{
				ChangeState(App->map->WorldToMap(enemyTarget->position.x, enemyTarget->position.y), AnimationType::ATTACK);
			}
			else
			{
				ChangeState(targetPosition, AnimationType::IDLE);
			}


		}
	}
	
}

void Unit::Init(int maxHealth)
{
	SetMaxHealth(maxHealth);
	HealthSystem::Init();

	targetPosition.ResetAsPosition();
	ChangeState(targetPosition, state);
}

void Unit::ChangeState(iPoint isoLookPosition, AnimationType newState)
{
	if (isoLookPosition == iPoint(-1, -1) && entPath.size() == 0)
	{
		currentAnim = App->entityManager->animations[unitType][AnimationType::IDLE][currentDirection];
	}
	else
	{
		currentDirection = getMovementDirection(isoLookPosition);
		if (App->entityManager->animations[unitType][newState][currentDirection].name != currentAnim.name)
			currentAnim = App->entityManager->animations[unitType][newState][currentDirection];
	}
	state = newState;
}

bool Unit::Draw(float dt)
{
	if (entPath.size() > 0 && targetPosition == iPoint(-1, -1))
	{
		targetPosition.x = entPath[0].x;
		targetPosition.y = entPath[0].y;

		iPoint rest = {(int)position.x, (int)position.y};

		iPoint fTarget = App->map->MapToWorld(targetPosition.x, targetPosition.y);
		fTarget += App->map->GetTilesHalfSize();

		directionToTarget = fTarget - rest;
		normalizedDirection = fPoint::Normalize((fPoint)directionToTarget);

		ChangeState(targetPosition, AnimationType::WALK);
		entPath.erase(entPath.begin(), entPath.begin() + 1);
	}


	if (targetPosition != iPoint(-1, -1))
		MoveToTarget();

	int num_current_anim = currentAnim.GetSprite();
	blitRect = { (int)(currentAnim.sprites[num_current_anim].rect.w / 1.5f), (int)(currentAnim.sprites[num_current_anim].rect.h / 1.5f) };

	//Collider update
	collisionRect.x = position.x - (collisionRect.w / 2);
	collisionRect.y = position.y;

	App->render->Blit(texture, position.x - blitRect.x / 2, position.y - blitRect.y, blitRect, &currentAnim.sprites[num_current_anim].rect, 1.f, flipState);

	//App->render->DrawQuad({(int)position.x, (int)position.y, 2, 2}, 0, 255, 0);

	if (displayDebug) 
	{
		if (entPath.size() > 0)
		{
			for (uint i = 0; i < entPath.size(); ++i)
			{
				iPoint pos = App->map->MapToWorld(entPath.at(i).x, entPath.at(i).y);
				App->render->Blit(App->scene->debugBlue_tex, pos.x, pos.y);
			}
		}
		App->render->DrawQuad(collisionRect, 0, 255, 0, 50);
	}

	return true;
}

void Unit::Action(Entity* entity)
{

	switch (unitType)
	{
		//Monks action are controled by Monastery so, its functionality is there ( on Monastery Building )
	}
}

Direction Unit::getMovementDirection(iPoint target)
{
	Direction dir = Direction::UP;

	iPoint temp = App->map->WorldToMap(position.x, position.y);

	target = App->map->MapToWorld(target.x, target.y);
	iPoint pos = App->map->MapToWorld(temp.x, temp.y);

	if (target.x >= position.x)
	{
		flipState = SDL_FLIP_HORIZONTAL;
	}
	else
	{
		flipState = SDL_FLIP_NONE;
	}

	if (target.x == pos.x && target.y < pos.y)
	{
		dir = Direction::UP;
	}
	else if (target.x == pos.x && target.y > pos.y)
	{
		dir = Direction::DOWN;
	}
	else if (target.x != pos.x && target.y == pos.y)
	{
		dir = Direction::LATERAL;
	}
	else if (target.x != pos.x && target.y > pos.y)
	{
		dir = Direction::DIAGONAL_DOWN;
	}
	else if (target.x != pos.x && target.y < pos.y)
	{
		dir = Direction::DIAGONAL_UP;
	}
	else
	{
		//Is the same place
		targetPosition.ResetAsPosition();
	}

	return dir;
}


void Unit::SetPath(const std::vector<iPoint> s_path)
{
	entPath = s_path;
}


void Unit::Kill(iPoint direction)
{
	ChangeState(direction, AnimationType::DIE);
}
void Unit::Draw_Life_Bar(bool enemy)
{
	SDL_Rect life_spriteRect = App->entityManager->unit_life_bar_back;
	iPoint pos;
	if (combat_unit == true) {
		CombatUnit* comb_unit = static_cast<CombatUnit*>(this);
		if (comb_unit->GetLevel() > 0)
			pos = { (int)position.x - 38, (int)position.y - 75 };
		else
			pos = { (int)position.x - 38, (int)position.y - 65 };
	}
	else 
		pos = { (int)position.x - 38, (int)position.y - 60 }; ////// IT WILL HAVE TO BE CHANGED WHEN NEW CREATURES ADDED

	App->render->Blit(App->gui->GetTexture(), pos.x, pos.y, &life_spriteRect);
	if (enemy == true)
		life_spriteRect = App->entityManager->unit_life_bar_front_enemy;
	else
		life_spriteRect = App->entityManager->unit_life_bar_front;
	float percentage_life = (float)GetHealth() / (float)GetMaxHealth();
	if (percentage_life < 0)
		percentage_life = 0;
	int sprite_rect_width = percentage_life * life_spriteRect.w;
	App->render->Blit(App->gui->GetTexture(), pos.x + 7, pos.y + 2, { sprite_rect_width, life_spriteRect.h },
		& life_spriteRect);
	life_spriteRect = App->entityManager->unit_life_bar_empty;
	App->render->Blit(App->gui->GetTexture(), pos.x, pos.y, &life_spriteRect);
}
void Unit::StateMachineActions(float dt)
{
	//LOG("%i", state);
	switch (state)
	{
	case AnimationType::ATTACK:
		if (currentAnim.current_sprite == currentAnim.num_sprites - 1)
		{
			CombatUnit* unit = (CombatUnit*)this;
			if (enemyTarget->RecieveDamage(unit->GetDamageValue()))
			{
				enemyTarget->Kill(App->map->WorldToMap(position.x, position.y));
				enemyTarget = nullptr;
				ChangeState(targetPosition, AnimationType::IDLE);
			}
		}

		break;
	case AnimationType::DIE:

		timeToDespawn -= dt;

		if (timeToDespawn <= 0) 
		{
			//App->entityManager->DeleteEntity(this);
			toDelete = true;
		}

		break;
	case AnimationType::HIT:
		break;
	case AnimationType::IDLE:
		break;
	case AnimationType::WALK:
		break;
	default:
		break;
	}
}


