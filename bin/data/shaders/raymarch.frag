#version 330

uniform vec2 screenResolution = vec2(320, 240);
uniform vec3 cameraPosition = vec3(0,0,-5);
uniform vec3 cameraUpVector = vec3(0,1,0);
uniform vec3 cameraLookTarget = vec3(0,0,0);
uniform int numIterations = 256;
uniform float maximumDepth = 1500.0f;
uniform vec4 skyColour = vec4(0.8f,0.8f,1.0f,1);
uniform float time;
uniform sampler2D noisetex;

in vec2 texCoord;
out vec4 finalColor;

precision highp float;

// Raymarching Shader

// Noise



float noise3D(vec3 p)
{
    p.z = fract(p.z)*2048.0;
    float iz = floor(p.z);
    float fz = fract(p.z);
    vec2 a_off = vec2(23.0, 29.0)*(iz)/2048.0;
    vec2 b_off = vec2(23.0, 29.0)*(iz+1.0)/2048.0;
    float a = texture(noisetex, p.xy + a_off, -999.0).r;
    float b = texture(noisetex, p.xy + b_off, -999.0).r;
    return mix(a, b, fz) - 0.5;
}

float noise_g(vec3 p)
{
    float v = 0.0;
    for (float i = 0.0; i < 5.0; i += 1.0)
        v += noise3D(p * pow(2.0, i)) * pow(0.6, i);
    return v;
}

//// Raymarching + CSG Functions


// Set of functions for representing distance-fields for shapes.
// The Y-Coordinate here is used for material/texturing information.
vec2 ShapeFlatFloor(vec3 worldPosition)
{
	return vec2(worldPosition.y + 10.0f, 0.0f);
}

vec2 CSG_Sphere( vec3 position, float size, vec3 worldPosition)
{
	return vec2(length(worldPosition - position) - size, 1.0f);
}

// The CSG functions are simple to calculate;
// In Union, Anything that is closer to the camera than the other will be shown instead.
vec2 CSG_Union(vec2 density1, vec2 density2)
{
	if(density1.x < density2.x)
	{
		return density1;
	}
	else
	{
		return density2;
	}
}

vec2 CSG_Subtract(vec2 density1, vec2 density2)
{
	if(density1.x < -density2.x)
	{
		density2.y = -2.0f;
		return -density2;
	}
	else
	{
		return density1;
	}
	
}

// This function calculates the density/distance field.
vec2 DistanceField(vec3 worldPosition)
{
	// First, start with a flat plane
	vec2 Density;

	Density = ShapeFlatFloor(worldPosition);

	// Then add some hills to the plane.
	Density.x += (noise_g(worldPosition * 0.0001f)) * 10.0f;
	Density.x += (noise_g(worldPosition * 0.0001f)) * 10.0f;
	
	
	
	
	// Add some objects.
	Density = CSG_Union(Density, CSG_Sphere(vec3(0,cos(time*0.01f)*5,-50), 20, worldPosition));
	Density = CSG_Subtract(Density, CSG_Sphere(vec3(0,-60,50), 20, worldPosition));



	return Density;
}

// Lighting functions

float softShadow(vec3 rayOrigin, vec3 rayDirection, float minimumDistance, float maximumDistance, float coefficient)
{
	float shadowResult = 1.0f;
	for(float t = minimumDistance; t < maximumDistance;)
	{
		float shadowDistance = DistanceField(rayOrigin + (rayDirection*t)).x;
		if(shadowDistance < 0.001)
		{
			return 0.0f;
		}
		shadowResult = min(shadowResult, coefficient * (shadowDistance/t));
		t += shadowDistance;
	}

	return shadowResult;
}

vec4 Lambertian(vec3 worldPosition, vec3 currentNormal)
{
	vec3 lightDirection = normalize(vec3(1.0, 1.0, 0.0));
	float lightIntensity = dot(currentNormal, lightDirection);
	if(lightIntensity > 0)
	{
		return vec4(1.0, 1.0, 1.0, 1.0) * lightIntensity;
	}
	return vec4(0.0, 0.0, 0.0, 1.0);
}

void main()
{
	// Distance-field raymarch shader.
	// Based on collected works by Inigo Quilez.
	// [link]

	vec2 screenPosition = gl_FragCoord.xy / screenResolution;
	
	screenPosition.y = 1.0f - screenPosition.y;
	screenPosition.x = 1.0f - screenPosition.x;
	vec2 screenPositionOffset = -1.0f + 2.0f * screenPosition;

	

	// First, Initialize Camera
	vec3 cameraDirection = normalize(cameraLookTarget - cameraPosition);
	vec3 cameraRight = normalize(cross(cameraUpVector, cameraDirection));
	vec3 cameraLowerBound = cross(cameraDirection, cameraRight);
	vec3 cameraStep = cameraPosition + cameraDirection;
	
	vec3 screenCoordinate = cameraStep + (screenPositionOffset.x * cameraRight * (screenResolution.x / screenResolution.y) * 0.55) + (screenPositionOffset.y * cameraLowerBound * 0.55);
	vec3 eyeCoordinate = normalize(screenCoordinate - cameraPosition);

	// Now, do raymarching.
	vec3 minDistance = vec3(0.2, 0, 0);



	vec2 currentDistance = minDistance.xy;
	currentDistance.x = 0.02;

	vec3 currentColour;
	vec3 currentHitPosition;
	vec3 currentHitNormal;

	float rayDistanceTravelled = 1.0f;

	// Evaluate distance field
	int i = 0;
	for(i = 0; i < numIterations; i++)
	{
		// If the ray hasn't hit anything yet, or if the step size becomes too small, stop here.
		if((abs(currentDistance.x) < 0.01) || (rayDistanceTravelled > maximumDepth))
		{
			break;
		}

		// Advance ray forwards.
		rayDistanceTravelled += currentDistance.x;

		// Update contact position
		currentHitPosition = cameraPosition + (eyeCoordinate * rayDistanceTravelled);

		// Check distance
		currentDistance = DistanceField(currentHitPosition);



	}

	// If the ray hit something
	if(rayDistanceTravelled < maximumDepth)
	{
		currentColour = vec3(0.1f, 0.85f, 0.1f);
		if(currentDistance.y > 0)
		{	
			currentColour = vec3(1.0f, 0.0f, 0.1f);
			if(currentDistance.y > 1)
			{
				currentColour = vec3(0.6f, 0.3f, 0.1f);
			}
		}
		

		// Calculate hit normal
		vec3 normalCalc = vec3(currentDistance.x - DistanceField((currentHitPosition - minDistance.xyy)).x,
							   currentDistance.x - DistanceField((currentHitPosition - minDistance.yxy)).x, 
							   currentDistance.x - DistanceField((currentHitPosition - minDistance.yyx)).x);

		currentHitNormal = normalize(normalCalc);

		

		// Lighting calculations
		vec4 ambientCol = vec4(0.0f, 0.0f, 0.1f, 1.0f);


		finalColor = (vec4(currentColour, 1.0f) * Lambertian(currentHitPosition, currentHitNormal));

		finalColor.rgb *= softShadow(currentHitPosition, normalize(vec3(1.0, 1.0, 0.0)), 1.0f, 1500.0f, 40);

		finalColor += ambientCol;

		// Fog Pass
		float fogCoefficient = 0.0f;

		// Fog depends on view distance
		fogCoefficient = (length(currentHitPosition - cameraPosition) / maximumDepth);

		float sunAmount = max( dot(normalize(currentHitPosition - cameraPosition), normalize(vec3(1.0, 1.0, 0.0))), 0.0f);

		vec4 fogColour = mix(skyColour, vec4(1.0f, 1.0f, 0.9f, 1.0f), pow(sunAmount, 8.0f));

		//gl_FragDepth?

		finalColor = mix(finalColor, fogColour, fogCoefficient);

	}
	else
	{
		// Ray didn't hit, return sky



		float sunAmount = max( dot(normalize(currentHitPosition - cameraPosition), normalize(vec3(1.0, 1.0, 0.0))), 0.0f);

		vec4 fogColour = mix(skyColour, vec4(1.0f, 1.0f, 0.9f, 1.0f), pow(sunAmount, 8.0f));

		

		finalColor = fogColour;
	}


	//finalColor = texture(noisetex, screenPosition);



}

