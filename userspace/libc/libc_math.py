from math import cos, pi, sin

with open("src/costable.h", "w") as f:
    f.write("double costable[] = {\n")
    j = 0
    p = 0.0
    while True:
        f.write("{:.20f}, ".format(cos(p)))
        j += 1
        p += 0.01
        if p > 2 * pi:
            break
    f.write("1.0 };\n")
    f.write(f"const int costable_size = {j + 1};\n")

    with open("src/sintable.h", "w") as f:
        f.write("double sintable[] = {\n")
        j = 0
        p = 0.0
        while True:
            f.write("{:.20f}, ".format(sin(p)))
            j += 1
            p += 0.01
            if p > 2 * pi:
                break
        f.write("1.0 };\n")
        f.write(f"const int sintable_size = {j + 1};\n")
