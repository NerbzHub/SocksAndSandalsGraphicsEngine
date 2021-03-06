/**
	ParticleEmitter.cpp

	Purpose: ParticleEmitter.cpp is the source file for the ParticleEmitter class.
			The Particle Emitter class allows the creation of particle effects
			in the scene.

	@author Nathan Nette
*/
#include "ParticleEmitter.h"

/**
	The Particle Emitter constructor assigns the values to 0
		as default.
*/
ParticleEmitter::ParticleEmitter()
	: m_particles(nullptr),
	m_firstDead(0),
	m_maxParticles(0),
	m_position(0, 0, 0),
	m_vao(0), m_vbo(0), m_ibo(0),
	m_vertexData(nullptr)
{
}

/**
	The Particle Emitter deconstructor deletes anything that
		needs to be cleaned up on termination of the program.
*/
ParticleEmitter::~ParticleEmitter()
{
	delete[] m_particles;
	delete[] m_vertexData;
	glDeleteVertexArrays(1, &m_vao);
	glDeleteBuffers(1, &m_vbo);
	glDeleteBuffers(1, &m_ibo);
}

/**
	initialise creates a new particle effect based on the
		params for this function.

		@param1 a_maxParticles is the maximum amount of particles
			that can exist at once.

		@param2  a_emitRate is the speed that particles emit at.

		@param3 a_lifetimeMin is the minimum life time of a particle.

		@param4 a_lifetimeMax is the maximum life time of a particle.

		@param5 a_velocityMin is the minimum of how fast the particles move.

		@param6 a_velocityMax is the maximum of how fast the particles move.

		@param7 a_startSize is the size of the particles when they first emit.

		@param8 a_endSize is the size of the particles when they end.

		@param9 a_startColour is the colour of the particle when they spawn.

		@param10 a_endColour is the colour of the particle when they end.
*/
void ParticleEmitter::initialise(unsigned int a_maxParticles,
	unsigned int a_emitRate,
	float a_lifetimeMin, float a_lifetimeMax,
	float a_velocityMin, float a_velocityMax,
	float a_startSize, float a_endSize,
	const glm::vec4& a_startColour, const glm::vec4& a_endColour)
{

	// Set up emit timers.
	m_emitTimer = 0;
	m_emitRate = 1.0f / a_emitRate;
	// Store all variables passed in.
	m_startColour = a_startColour;
	m_endColour = a_endColour;
	m_startSize = a_startSize;
	m_endSize = a_endSize;
	m_velocityMin = a_velocityMin;
	m_velocityMax = a_velocityMax;
	m_lifespanMin = a_lifetimeMin;
	m_lifespanMax = a_lifetimeMax;
	m_maxParticles = a_maxParticles;

	// Create particle array.
	m_particles = new Particle[m_maxParticles];
	m_firstDead = 0;

	// Create the array of vertices for the particles:
	// 4 vertices per particle for a quad
	// fill be filled during update.
	m_vertexData = new ParticleVertex[m_maxParticles * 4];

	// Create the index buffer data for the particles:
	// 6 indices per quad of 2 triangles
	// fill it now as it never changes.
	unsigned int* indexData = new unsigned int[m_maxParticles * 6];
	for (unsigned int i = 0; i < m_maxParticles; ++i) {
		indexData[i * 6 + 0] = i * 4 + 0;
		indexData[i * 6 + 1] = i * 4 + 1;
		indexData[i * 6 + 2] = i * 4 + 2;
		indexData[i * 6 + 3] = i * 4 + 0;
		indexData[i * 6 + 4] = i * 4 + 2;
		indexData[i * 6 + 5] = i * 4 + 3;
	}
	// Create opengl buffers.
	glGenVertexArrays(1, &m_vao);
	glBindVertexArray(m_vao);
	glGenBuffers(1, &m_vbo);
	glGenBuffers(1, &m_ibo);
	glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
	glBufferData(GL_ARRAY_BUFFER, m_maxParticles * 4 *
		sizeof(ParticleVertex), m_vertexData,
		GL_DYNAMIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ibo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, m_maxParticles * 6 *
		sizeof(unsigned int), indexData, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0); // position
	glEnableVertexAttribArray(1); // colour
	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE,
		sizeof(ParticleVertex), 0);
	glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE,
		sizeof(ParticleVertex), ((char*)0) + 16);
	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	delete[] indexData;
}

/**
	emit is the function to be called to spawn a new particle.
		it only runs if there is currently less than the maximum
		amount. This means that when one dies, a new one spawn
		straight away.
*/
void ParticleEmitter::emit()
{
	// Only emit if there is a dead particle to use.
	if (m_firstDead >= m_maxParticles)
		return;
	// Resurrect the first dead particle.
	Particle& particle = m_particles[m_firstDead++];
	// Assign its starting position.
	particle.position = m_position;
	// Randomise its lifespan.
	particle.lifetime = 0;
	particle.lifespan = (rand() / (float)RAND_MAX) *
		(m_lifespanMax - m_lifespanMin) + m_lifespanMin;
	// Set starting size and colour.
	particle.colour = m_startColour;
	particle.size = m_startSize;
	// Randomise velocity direction and strength.
	float velocity = (rand() / (float)RAND_MAX) *
		(m_velocityMax - m_velocityMin) + m_velocityMin;
	particle.velocity.x = (rand() / (float)RAND_MAX) * 2 - 1;
	particle.velocity.y = (rand() / (float)RAND_MAX) * 2 - 1;
	particle.velocity.z = (rand() / (float)RAND_MAX) * 2 - 1;
	particle.velocity = glm::normalize(particle.velocity) *
		velocity;
}

/**
	update is called every frame. It does all of the necessary
		math to move every particle and change their colour.
		Also checks to see how many exist. If there are less
		than the maximum, call emit.
*/
void ParticleEmitter::update(float a_deltaTime,
	const glm::mat4& a_cameraTransform)
{
	using glm::vec3;
	using glm::vec4;

	// Spawn particles.
	m_emitTimer += a_deltaTime;

	while (m_emitTimer > m_emitRate) {
		emit();
		m_emitTimer -= m_emitRate;
	}
	unsigned int quad = 0;

	// Update particles and turn live particles into billboard quads.
	for (unsigned int i = 0; i < m_firstDead; i++)
	{
		Particle* particle = &m_particles[i];
		particle->lifetime += a_deltaTime;

		if (particle->lifetime >= particle->lifespan) {
			// Swap last alive with this one.
			*particle = m_particles[m_firstDead - 1];
			m_firstDead--;
		}

		else {
			// Move particle.
			particle->position += particle->velocity * a_deltaTime;
			// Size particle.
			particle->size = glm::mix(m_startSize, m_endSize,
				particle->lifetime / particle->lifespan);
			// Colour particle.
			particle->colour = glm::mix(m_startColour, m_endColour,
				particle->lifetime / particle->lifespan);
			// Make a quad the correct size and colour.
			float halfSize = particle->size * 0.5f;
			m_vertexData[quad * 4 + 0].position = glm::vec4(halfSize,
				halfSize, 0, 1);
			m_vertexData[quad * 4 + 0].colour = particle->colour;
			m_vertexData[quad * 4 + 1].position = glm::vec4(-halfSize,
				halfSize, 0, 1);
			m_vertexData[quad * 4 + 1].colour = particle->colour;
			m_vertexData[quad * 4 + 2].position = glm::vec4(-halfSize,
				-halfSize, 0, 1);
			m_vertexData[quad * 4 + 2].colour = particle->colour;
			m_vertexData[quad * 4 + 3].position = glm::vec4(halfSize,
				-halfSize, 0, 1);
			m_vertexData[quad * 4 + 3].colour = particle->colour;

			// Create billboard transform.
			glm::vec3 zAxis = glm::normalize(glm::vec3(a_cameraTransform[3]) - particle->position);
			glm::vec3 xAxis = glm::cross(glm::vec3(a_cameraTransform[1]), zAxis);
			glm::vec3 yAxis = glm::cross(zAxis, xAxis);
			glm::mat4 billboard(vec4(xAxis, 0),
				glm::vec4(yAxis, 0),
				glm::vec4(zAxis, 0),
				glm::vec4(0, 0, 0, 1));
			m_vertexData[quad * 4 + 0].position = billboard *
				m_vertexData[quad * 4 + 0].position +
				glm::vec4(particle->position, 0);
			m_vertexData[quad * 4 + 1].position = billboard *
				m_vertexData[quad * 4 + 1].position +
				glm::vec4(particle->position, 0);
			m_vertexData[quad * 4 + 2].position = billboard *
				m_vertexData[quad * 4 + 2].position +
				glm::vec4(particle->position, 0);
			m_vertexData[quad * 4 + 3].position = billboard *
				m_vertexData[quad * 4 + 3].position +
				glm::vec4(particle->position, 0);
			++quad;
		}
	}
}

/**
	draw is the function that actually renders them to the screen.
*/
void ParticleEmitter::draw()
{
	// Sync the particle vertex buffer.
	// Based on how many alive particles there are.
	glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
	glBufferSubData(GL_ARRAY_BUFFER, 0, m_firstDead * 4 *
		sizeof(ParticleVertex), m_vertexData);
	// Draw particles.
	glBindVertexArray(m_vao);
	glDrawElements(GL_TRIANGLES, m_firstDead * 6, GL_UNSIGNED_INT, 0);
}
