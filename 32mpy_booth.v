module lab(CLK, RST, din,addr, ctrl,Partial_Product, Product_Valid);
input CLK, RST;
input [2:0]ctrl;
input [15:0]addr;
input signed [31:0] din;
output reg [31:0] Partial_Product;
output reg Product_Valid;

reg signed[31:0]	in_a;	// Multiplicand
reg signed[31:0]	in_b;// Multiplier

reg [31:0] Mplicand;
reg signed [63:0] Product;
//reg [31:0] Mplier;
reg [6:0]	Counter ;
reg	sign;
reg Mythicalbit;

//Counter
always@(posedge CLK or negedge RST)
begin
	if(!RST)
		Counter <=6'b0;
	else if(ctrl[0])
		Counter <=6'b0;
	else if(Counter<=6'd33)
		Counter <= Counter +6'b1;
end

always@(posedge CLK or negedge RST)
begin
	if(!RST) begin
		in_a <=32'd0;
		in_b <=32'd0;
	end
	else if(addr[15:8]==8'h00)begin
		in_a <=din;
		in_b <=din;
	end
end

always@(posedge CLK or negedge RST)
begin
	if(!RST)begin
		Partial_Product<=32'd0;
	end
	else if(ctrl[2])begin
		Partial_Product<=Product[31:0];
	end
	else if(ctrl[1])begin
		Partial_Product<=Product[63:32];
	end
end

//Multiplier
always @(posedge CLK or negedge RST)
begin
	//??å?‹å?–æ•¸??
	if(!RST) begin
		Product  <= 64'b0;
		Mplicand <= 32'b0;
		Mythicalbit <= 1'b0;
		//Mplier   <= 32'b0;	
	end 
	
	//è¼¸å…¥ä¹˜æ•¸??‡è¢«ä¹˜æ•¸
	else if(Counter == 6'd0) begin
		Mplicand <= in_a;
		Product <={32'b0,in_b};
		Mythicalbit <= 1'b0;
	end 
	
	//ä¹˜æ?•è?‡æ•¸?¼ç§»ä½?
	/* write down your design below */
	else if(Counter <=7'd32) 
	begin
	
		if(Product[0]==1'b0 && Mythicalbit ==1'b1) //Product ??ä½Žä?ç‚º0 ä¸? Mythicalbit ?‚º1
		begin//??šå??: ??Šè¢«ä¹˜æ•¸??? Productå·¦å?Šç›¸???, å­˜å?? Productå·¦å??
			Product = Product + {Mplicand,32'b0};
		end
		
		if(Product[0]==1'b1 && Mythicalbit==1'b0)//Product ??ä½Žä?ç‚º1 ä¸? Mythicalbit ?‚º0
		begin//??šæ??: ??Šè¢«ä¹˜æ•¸??? Productå·¦å?Šç›¸æ¸?, å­˜å?? Productå·¦å??
			Product = Product - {Mplicand,32'b0};
		end
		
		Mythicalbit = Product[0];
		
		Product = Product >>> 1;
	
	end 
	/* write down your design upon */
end

//?ˆ¤?–·è¼¸å‡º
always @(posedge CLK or negedge RST)
begin
	if(!RST)
		Product_Valid <=1'b1;
	else if(Counter>=6'd32)
		Product_Valid <=1'b0;
	else
		Product_Valid <=1'b0;
end

endmodule
