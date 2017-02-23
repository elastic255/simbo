        function [ str ] = int2ip(in )

            if iscell(in)
                A = int64(in{1});
            else
                A = int64(in);
            end
            B = int64([0,0,0,0]);
            for i=1:4
                B(i) = idivide( A, int64(256^(4-i)), 'floor');
                A = A - B(i)*int64(256^(4-i));

            end
            str = [num2str(B(1)) '.' num2str(B(2)) '.' num2str(B(3)) '.' num2str(B(4))];
        end