float3 SurfaceNormal(SceneTriangle shape, float3 face, float u, float v) {
    if (shape.has_normals == 0) return face;
    float3 blended = shape.normal_a * (1.0 - u - v) + shape.normal_b * u + shape.normal_c * v;
    return dot(blended, blended) > 0.0 ? normalize(blended) : face;
}

bool IntersectTriangle(SceneTriangle shape, Ray ray, float minimum, float maximum, out Hit hit) {
    hit = (Hit)0;
    float3 first_edge = shape.b - shape.a;
    float3 second_edge = shape.c - shape.a;
    float3 perpendicular = cross(ray.direction, second_edge);
    float determinant = dot(first_edge, perpendicular);
    if (abs(determinant) < determinant_epsilon) return false;
    float inverse = 1.0 / determinant;
    float3 offset = ray.origin - shape.a;
    float u = dot(offset, perpendicular) * inverse;
    if (u < 0.0 || u > 1.0) return false;
    float3 cross_offset = cross(offset, first_edge);
    float v = dot(ray.direction, cross_offset) * inverse;
    if (v < 0.0 || u + v > 1.0) return false;
    float distance = dot(second_edge, cross_offset) * inverse;
    if (distance < minimum || distance > maximum) return false;
    float3 face = cross(first_edge, second_edge);
    if (dot(face, face) == 0.0) return false;
    hit.distance = distance;
    hit.position = ray.origin + ray.direction * distance;
    hit.normal = SurfaceNormal(shape, normalize(face), u, v);
    if (dot(hit.normal, ray.direction) > 0.0) hit.normal = -hit.normal;
    hit.material = shape.material;
    return true;
}

bool EntersBox(SceneNode node, Ray ray, float3 reciprocal, float minimum, float maximum, out float entry) {
    entry = minimum;
    if (maximum < minimum) return false;
    [unroll]
    for (uint axis = 0; axis < 3; ++axis) {
        float origin = ray.origin[axis];
        if (ray.direction[axis] == 0.0) {
            if (origin < node.bounds_min[axis] || origin > node.bounds_max[axis]) return false;
            continue;
        }
        float first = (node.bounds_min[axis] - origin) * reciprocal[axis];
        float last = (node.bounds_max[axis] - origin) * reciprocal[axis];
        minimum = max(minimum, min(first, last));
        maximum = min(maximum, max(first, last));
        if (maximum < minimum) return false;
    }
    entry = minimum;
    return true;
}

bool TraverseScene(Ray ray, float minimum, float maximum, bool any_hit, out Hit nearest) {
    nearest = (Hit)0;
    if (node_count == 0) return false;
    float3 reciprocal = 1.0 / ray.direction;
    float root_entry;
    if (!EntersBox(nodes[0], ray, reciprocal, minimum, maximum, root_entry)) return false;
    uint stack[stack_size];
    stack[0] = 0;
    uint top = 1;
    bool found = false;
    float limit = maximum;
    [loop]
    while (top > 0) {
        --top;
        SceneNode node = nodes[stack[top]];
        if (node.primitive_count == 0) {
            float left_entry;
            float right_entry;
            bool left = EntersBox(nodes[node.left_child], ray, reciprocal, minimum, limit, left_entry);
            bool right = EntersBox(nodes[node.right_child], ray, reciprocal, minimum, limit, right_entry);
            bool left_first = left && (!right || left_entry <= right_entry);
            uint hits = uint(left) + uint(right);
            if (hits == 2 && top + 2 <= stack_size) {
                stack[top] = left_first ? node.right_child : node.left_child;
                ++top;
            }
            if (hits >= 1 && top < stack_size) {
                stack[top] = left_first ? node.left_child : node.right_child;
                ++top;
            }
            continue;
        }
        [loop]
        for (uint offset = 0; offset < node.primitive_count; ++offset) {
            Hit candidate;
            if (!IntersectTriangle(triangles[node.first_primitive + offset], ray, minimum, limit, candidate))
                continue;
            nearest = candidate;
            if (any_hit) return true;
            limit = candidate.distance;
            found = true;
        }
    }
    return found;
}
