lp = require("l_profiler")

lp.start()

function test_a()
    local count = 0
    for i = 1, 100 do
        count = (count + i) * i
    end
    return count
end

function test_b()
    for i = 1, 10 do
        test_a()
    end
end

function test_c()
    return test_b()
end

test_c()
test_b()
test_c()

print(lp.dump_string())
lp.stop()
