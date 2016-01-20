
/*
 * mover kan gedefined worden,
 *[03-Mar-15 10:16:30 PM] Marcel Smit:        if (fields[0] == "sprite")
 *        mover->setSprite(fields[1].c_str());
 *       if (fields[0] == "x1")
 *        mover->m_x1 = Parse::Int32(fields[1]);
 *       if (fields[0] == "y1")
 *        mover->m_y1 = Parse::Int32(fields[1]);
 *       if (fields[0] == "x2")
 *        mover->m_x2 = Parse::Int32(fields[1]);
 *       if (fields[0] == "y2")
 *        mover->m_y2 = Parse::Int32(fields[1]);
 *       if (fields[0] == "speed")
 *        mover->m_speed = Parse::Int32(fields[1]);
 *[03-Mar-15 10:16:40 PM] Marcel Smit: "de officieuze spec"
*/

/*
[4:48:24 PM] Marcel Smit: de beste referentie is de code. ik zou als ik jou was iets maken waarbij je in de editor support hebt voor rectangles. ik noem de coordinates altjd x1, y1, x2, y2 incode.. als dat niet zo is moet je me vertellen dat te fixen. dus 't zou in ieder geval eenvoudig moeten zijn om altijd die weg te schrijven. vervolgens heb je iets nodig waarme je property lists kunt definen per object type, en vervolgens generieke UI elementen dynamisch toevoegt per property. met wat simpele book keeping zou je op die manier alle object types eenvoudig kunnen toevoegen, seralializen en kunnen editen. "documentatie" voor de object types:

   std::string type = d.getString("type", "");

   if (type == "mover")
   {
	Mover * mover = 0;

	for (int i = 0; i < MAX_MOVERS; ++i)
	{
	 if (!m_movers[i].m_isActive)
	 {
	  mover = &m_movers[i];
	  break;
	 }
	}

	if (mover == 0)
	 LOG_ERR("too many movers!");
	else
	{
	 mover->m_isActive = true;

	 mover->setSprite(d.getString("sprite", "").c_str());
	 mover->m_x1 = d.getInt("x1", 0);
	 mover->m_y1 = d.getInt("y1", 0);
	 mover->m_x2 = d.getInt("x2", 0);
	 mover->m_y2 = d.getInt("y2", 0);
	 mover->m_speed = d.getInt("speed", 0);
	}
   }
   else if (type == "light")
   {
	Light * light = 0;

	for (int i = 0; i < MAX_LIGHTS; ++i)
	{
	 if (!m_lights[i].m_isAlive)
	 {
	  light = &m_lights[i];
	  break;
	 }
	}

	if (light == 0)
	 LOG_ERR("too many torches!");
	else
	{
	 light->m_isAlive = true;

	 light->m_pos[0] = d.getInt("x", 0);
	 light->m_pos[1] = d.getInt("y", 0);

	 const Color color = parseColor(d.getString("color", "ffffffff").c_str());
	 light->m_color[0] = color.r;
	 light->m_color[1] = color.g;
	 light->m_color[2] = color.b;
	 light->m_color[3] = color.a;
	}
   }
   else if (type == "tilesprite")
   {
	TileSprite * tileSprite = 0;

	for (int i = 0; i < MAX_TILE_SPRITES; ++i)
	{
	 if (!m_tileSprites[i].m_isAlive)
	 {
	  tileSprite = &m_tileSprites[i];
	  break;
	 }
	}

	if (tileSprite == 0)
	 LOG_ERR("too many tile sprites!");
	else
	{
	 tileSprite->setup(
	  d.getString("sprite", "").c_str(),
	  d.getInt("px", 0),
	  d.getInt("py", 0),
	  d.getInt("x1", 0),
	  d.getInt("y1", 0),
	  d.getInt("x2", 0),
	  d.getInt("y2", 0));
	 tileSprite->m_transition.parse(d);
	}
   }
   else if (type == "tiletransition")
   {
	if (m_arena.m_numTileTransitions < MAX_TILE_TRANSITIONS)
	{
	 Arena::TileTransition & tileTransition = m_arena.m_tileTransitions[m_arena.m_numTileTransitions++];

	 tileTransition.setup(
	  d.getInt("x1", 0) / BLOCK_SX,
	  d.getInt("y1", 0) / BLOCK_SY,
	  d.getInt("x2", 0) / BLOCK_SX,
	  d.getInt("y2", 0) / BLOCK_SY,
	  d);
	}
	else
	{
	 LOG_ERR("too many tile transitions!");
	}
   }
   else if (type == "particleeffect")
   {
	ParticleEffect * particleEffect = 0;

	for (int i = 0; i < MAX_PARTICLE_EFFECTS; ++i)
	{
	 if (!m_particleEffects[i].m_data.m_isActive)
	 {
	  particleEffect = &m_particleEffects[i];
	  break;
	 }
	}

	if (particleEffect == 0)
	 LOG_ERR("too many particle effects!");
	else
	{
	 particleEffect->setup(
	  d.getString("file", "").c_str(),
	  d.getInt("x", 0),
	  d.getInt("y", 0));
	}
   }
   else if (type == "portal")
   {
	Portal * portal = 0;

	for (int i = 0; i < MAX_PORTALS; ++i)
	{
	 if (!m_portals[i].m_isAlive)
	 {
	  portal = &m_portals[i];
	  break;
	 }
	}

	if (portal == 0)
	 LOG_ERR("too many portals!");
	else
	{
	 portal->setup(
	  d.getInt("x1", 0),
	  d.getInt("y1", 0),
	  d.getInt("x2", 0),
	  d.getInt("y2", 0),
	  d.getInt("key", 0));
	}
   }
   else if (type == "pickupspawn")
   {
	PickupSpawner * spawner = 0;

	for (int i = 0; i < MAX_PICKUP_SPAWNERS; ++i)
	{
	 if (!m_pickupSpawners[i].m_isAlive)
	 {
	  spawner = &m_pickupSpawners[i];
	  break;
	 }
	}

	if (spawner == 0)
	 LOG_ERR("too many pickup spawners!");
	else
	{
	 PickupType type;
	 if (parsePickupType(d.getString("pickup", 0).c_str(), type))
	 {
	  spawner->setup(
	   d.getInt("x1", 0),
	   d.getInt("y1", 0),
	   d.getInt("x2", 0),
	   d.getInt("y2", 0),
	   type,
	   d.getInt("interval", 0));
	 }
	}
   }
[4:48:47 PM] Marcel Smit: dit zijn trouwens de todos die in player.cpp staan voor de level editor:
- level setting: max zoom factor
- level setting: zoom restriction near borders. not needed when no level wrap
- level setting: level theme / background
- level setting: wrap around yes/no? could be done more easily by adding collision around the map
- level editor: add tile sprite placement
- level editor: add tile transition placement
- level editor: add teleporter placement
- level editor: add light source placement
- level editor: add particle emitter placement
- level editor: add moving platform placement
- level editor: add pickup spawner placement
- level editor: add ability to set map name. perhaps description
- level editor: add setting for ambient lighting
- level editor: add checkboxes for allowed (or disabled?) level events
- level editor: add text box for map name





  Level properties popup

  layers als textures
  1 scene met buttons


  */
