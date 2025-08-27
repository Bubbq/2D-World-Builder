#include <stdio.h>

int bound_value_to_interval(const int min, const int max, const int value)
{
    return (value > max) ? 
           max : 
           (value < min) ? min : value;
}

int main()
{
    printf("%d\n", bound_value_to_interval(0, 10, 9));
    printf("%d\n", bound_value_to_interval(0, 10, -1));
    printf("%d\n", bound_value_to_interval(0, 10, 11));
}